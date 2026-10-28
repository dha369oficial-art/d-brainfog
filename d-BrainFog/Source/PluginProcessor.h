#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "DSP/BrainFogEngine.h"
#include "DSP/Presets.h"

class BrainFogProcessor : public juce::AudioProcessor
{
public:
    BrainFogProcessor();
    ~BrainFogProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "d-BrainFog"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return bf::kNumFactoryPresets; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // presets de usuario (.dbfpreset = XML con el estado de los parametros)
    bool saveUserPreset (const juce::File& file);
    bool loadUserPreset (const juce::File& file);
    static juce::File userPresetDirectory();

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> meterPeak { 0.0f };       // pico del ultimo bloque (lo lee el editor)
    juce::String currentPresetName { "After Dark" };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void applyFactoryPreset (int index);

    bf::Engine engine;
    std::array<std::atomic<float>*, bf::P_COUNT> raw {};
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrainFogProcessor)
};
