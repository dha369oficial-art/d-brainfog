#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout BrainFogProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < bf::P_COUNT; ++i)
    {
        const auto& d = bf::kParams[i];
        const juce::ParameterID pid { d.id, 1 };
        switch (d.kind)
        {
            case bf::K_FLOAT:
            {
                juce::NormalisableRange<float> range (d.mn, d.mx, 0.0f);
                if (d.mid > d.mn && d.mid < d.mx) range.setSkewForCentre (d.mid);
                layout.add (std::make_unique<juce::AudioParameterFloat> (
                    pid, d.name, range, d.def,
                    juce::AudioParameterFloatAttributes().withLabel (d.unit)));
                break;
            }
            case bf::K_INT:
                layout.add (std::make_unique<juce::AudioParameterInt> (
                    pid, d.name, (int) d.mn, (int) d.mx, (int) d.def,
                    juce::AudioParameterIntAttributes().withLabel (d.unit)));
                break;
            case bf::K_BOOL:
                layout.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.def > 0.5f));
                break;
            case bf::K_CHOICE:
                layout.add (std::make_unique<juce::AudioParameterChoice> (
                    pid, d.name, juce::StringArray::fromTokens (d.choices, "|", ""), (int) d.def));
                break;
        }
    }
    return layout;
}

BrainFogProcessor::BrainFogProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < bf::P_COUNT; ++i)
        raw[(size_t) i] = apvts.getRawParameterValue (bf::kParams[i].id);
    applyFactoryPreset (0);
}

bool BrainFogProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void BrainFogProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);           // aqui (y solo aqui) se reserva memoria
}

void BrainFogProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (buffer.getNumChannels() < 2) return;

    float p[bf::P_COUNT];
    for (int i = 0; i < bf::P_COUNT; ++i) p[i] = raw[(size_t) i]->load();
    engine.setParams (p);

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                engine.setTempo (*bpm);

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getWritePointer (1);

    int cursor = 0;
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int t = juce::jlimit (0, n, meta.samplePosition);
        if (t > cursor) { engine.process (l + cursor, r + cursor, t - cursor); cursor = t; }

        if (msg.isNoteOn())                                   engine.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff())                             engine.noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())  engine.allNotesOff();
        else if (msg.isPitchWheel())                          engine.pitchBend ((float) (msg.getPitchWheelValue() - 8192) / 8192.0f);
        else if (msg.isControllerOfType (64))                 engine.sustain (msg.getControllerValue() >= 64);
    }
    if (cursor < n) engine.process (l + cursor, r + cursor, n - cursor);

    meterPeak.store (juce::jmax (buffer.getMagnitude (0, 0, n), buffer.getMagnitude (1, 0, n)));
}

// ---------------------------------------------------------------- presets
void BrainFogProcessor::applyFactoryPreset (int index)
{
    index = juce::jlimit (0, bf::kNumFactoryPresets - 1, index);
    float v[bf::P_COUNT];
    bf::factoryPresetValues (index, v);
    for (int i = 0; i < bf::P_COUNT; ++i)
        if (auto* prm = apvts.getParameter (bf::kParams[i].id))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (v[i]));
            prm->endChangeGesture();
        }
    currentProgram = index;
    currentPresetName = bf::kFactoryPresetNames[index];
}

void BrainFogProcessor::setCurrentProgram (int index) { applyFactoryPreset (index); }

const juce::String BrainFogProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, bf::kNumFactoryPresets) ? juce::String (bf::kFactoryPresetNames[index]) : juce::String();
}

juce::File BrainFogProcessor::userPresetDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("DHA369").getChildFile ("d-BrainFog").getChildFile ("Presets");
}

bool BrainFogProcessor::saveUserPreset (const juce::File& file)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", file.getFileNameWithoutExtension(), nullptr);
    if (auto xml = state.createXml())
    {
        file.getParentDirectory().createDirectory();
        if (xml->writeTo (file)) { currentPresetName = file.getFileNameWithoutExtension(); return true; }
    }
    return false;
}

bool BrainFogProcessor::loadUserPreset (const juce::File& file)
{
    if (auto xml = juce::parseXML (file))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            currentPresetName = file.getFileNameWithoutExtension();
            return true;
        }
    return false;
}

// ---------------------------------------------------------------- estado del proyecto
void BrainFogProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", currentPresetName, nullptr);
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void BrainFogProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentPresetName = tree.getProperty ("presetName", "After Dark").toString();
            currentProgram = juce::jlimit (0, bf::kNumFactoryPresets - 1, (int) tree.getProperty ("program", 0));
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* BrainFogProcessor::createEditor() { return new BrainFogEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BrainFogProcessor(); }
