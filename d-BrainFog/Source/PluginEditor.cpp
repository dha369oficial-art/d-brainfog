#include "PluginEditor.h"

// ------------------------------------------------------------ look & feel
BrainFogLookAndFeel::BrainFogLookAndFeel()
{
    using namespace bfcol;
    setColour (juce::ComboBox::backgroundColourId, panel.darker (0.4f));
    setColour (juce::ComboBox::textColourId, bone);
    setColour (juce::ComboBox::outlineColourId, edge);
    setColour (juce::ComboBox::arrowColourId, blood);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff101012));
    setColour (juce::PopupMenu::textColourId, bone);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, blood);
    setColour (juce::PopupMenu::highlightedTextColourId, bone);
    setColour (juce::Label::textColourId, dim);
    setColour (juce::TextButton::buttonColourId, panel.brighter (0.1f));
    setColour (juce::TextButton::textColourOffId, bone);
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xff101012));
    setColour (juce::BubbleComponent::outlineColourId, blood);
    setColour (juce::TooltipWindow::textColourId, bone);
}

void BrainFogLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                            float startAngle, float endAngle, juce::Slider& s)
{
    using namespace bfcol;
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float radius = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;

    juce::Path track;
    track.addCentredArc (c.x, c.y, radius - 2.0f, radius - 2.0f, 0.0f, startAngle, endAngle, true);
    g.setColour (edge);
    g.strokePath (track, juce::PathStrokeType (3.0f));

    juce::Path val;
    const float from = bipolar ? startAngle + (float) s.valueToProportionOfLength (0.0) * (endAngle - startAngle) : startAngle;
    val.addCentredArc (c.x, c.y, radius - 2.0f, radius - 2.0f, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
    g.setColour (s.isEnabled() ? blood : dim);
    g.strokePath (val, juce::PathStrokeType (3.5f));

    const float kr = radius - 7.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2a2d), c.x, c.y - kr, juce::Colour (0xff0e0e10), c.x, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
    g.setColour (edge);
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.0f);

    juce::Path ptr;
    ptr.addRectangle (-1.5f, -kr + 2.0f, 3.0f, kr * 0.55f);
    g.setColour (bone);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

void BrainFogLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool)
{
    using namespace bfcol;
    auto r = b.getLocalBounds().toFloat().reduced (3.0f);
    const bool on = b.getToggleState();
    g.setColour (on ? blood : panel.darker (0.5f));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (hl ? bone : edge);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    g.setColour (on ? bone : dim);
    g.setFont (juce::Font (juce::FontOptions (10.5f)));
    g.drawText (b.getButtonText(), b.getLocalBounds().reduced (2), juce::Justification::centred);
}

// ------------------------------------------------------------ editor
static constexpr int kW = 1000, kH = 712;

BrainFogEditor::BrainFogEditor (BrainFogProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (kW, kH);
    buildGrunge (kW, kH);

    for (int i = 0; i < bf::P_COUNT; ++i)
    {
        if (i == bf::P_VOLUME) continue;           // el volumen maestro se crea aparte (cabecera)
        const auto& d = bf::kParams[i];
        auto& c = ctls[(size_t) i];
        switch (d.kind)
        {
            case bf::K_FLOAT:
            case bf::K_INT:
                c.slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
                c.slider->setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
                c.slider->setPopupDisplayEnabled (true, true, this);
                addAndMakeVisible (*c.slider);
                c.sa = std::make_unique<SliderAtt> (proc.apvts, d.id, *c.slider);
                break;
            case bf::K_CHOICE:
                c.combo = std::make_unique<juce::ComboBox>();
                c.combo->addItemList (juce::StringArray::fromTokens (d.choices, "|", ""), 1);
                addAndMakeVisible (*c.combo);
                c.ca = std::make_unique<ComboAtt> (proc.apvts, d.id, *c.combo);
                break;
            case bf::K_BOOL:
                c.toggle = std::make_unique<juce::ToggleButton> (d.name);
                addAndMakeVisible (*c.toggle);
                c.ba = std::make_unique<ButtonAtt> (proc.apvts, d.id, *c.toggle);
                break;
        }
        if (d.kind != bf::K_BOOL)
        {
            c.label = std::make_unique<juce::Label> ("", d.name);
            c.label->setJustificationType (juce::Justification::centred);
            c.label->setFont (juce::Font (juce::FontOptions (10.5f)));
            c.label->setInterceptsMouseClicks (false, false);
            addAndMakeVisible (*c.label);
        }
    }

    using namespace bf;
    sections = {
        { "TONE MACROS", { P_TONE, P_CUTOFF, P_GRIT, P_DRIFT, P_DARKSPACE, P_SPACEMIX }, {}, 104, 100 },
        { "OSCILLATORS", { P_O1WAVE, P_O1LEVEL, P_PW, P_O2WAVE, P_O2LEVEL, P_O2SEMI, P_DETUNE, P_SUB, P_NOISE }, {}, 66, 84 },
        { "FILTER",      { P_RES, P_FENV, P_KEYTRACK }, {}, 66, 84 },
        { "VOICE",       { P_POLY, P_LEGATO, P_GLIDE, P_PBRANGE }, {}, 66, 84 },
        { "AMP ENV",     { P_AA, P_AD, P_AS, P_AR }, {}, 66, 84 },
        { "FILTER ENV",  { P_FA, P_FD, P_FS, P_FR }, {}, 66, 84 },
        { "LFO",         { P_LFOSHAPE, P_LFOSYNC, P_LFODIV, P_LFORATE, P_LFOPITCH, P_LFOCUT, P_LFOPW }, {}, 66, 84 },
        { "EFFECTS",     { P_DISTON, P_DISTMIX, P_CHON, P_CHRATE, P_CHDEPTH, P_CHMIX, P_DLON, P_DLTIME, P_DLFB, P_DLMIX, P_RVON, P_RVSIZE }, {}, 66, 84 },
    };
    // volumen maestro va en la cabecera
    {
        auto& c = ctls[(size_t) P_VOLUME];
        c.slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
        c.slider->setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        c.slider->setPopupDisplayEnabled (true, true, this);
        addAndMakeVisible (*c.slider);
        c.sa = std::make_unique<SliderAtt> (proc.apvts, kParams[P_VOLUME].id, *c.slider);
        c.label = std::make_unique<juce::Label> ("", "Master");
        c.label->setJustificationType (juce::Justification::centred);
        c.label->setFont (juce::Font (juce::FontOptions (10.5f)));
        addAndMakeVisible (*c.label);
    }

    for (int i = 0; i < kNumFactoryPresets; ++i) presetBox.addItem (kFactoryPresetNames[i], i + 1);
    presetBox.setSelectedItemIndex (juce::jmax (0, proc.getCurrentProgram()), juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    presetLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    presetLabel.setColour (juce::Label::textColourId, bfcol::dim);
    addAndMakeVisible (presetLabel);

    saveBtn.onClick = [this]
    {
        auto dir = BrainFogProcessor::userPresetDirectory(); dir.createDirectory();
        chooser = std::make_unique<juce::FileChooser> ("Save d-BrainFog preset", dir, "*.dbfpreset");
        juce::Component::SafePointer<BrainFogEditor> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [safe] (const juce::FileChooser& fc)
                              {
                                  if (safe == nullptr) return;
                                  auto f = fc.getResult();
                                  if (f != juce::File()) safe->proc.saveUserPreset (f.withFileExtension ("dbfpreset"));
                              });
    };
    loadBtn.onClick = [this]
    {
        auto dir = BrainFogProcessor::userPresetDirectory(); dir.createDirectory();
        chooser = std::make_unique<juce::FileChooser> ("Load d-BrainFog preset", dir, "*.dbfpreset");
        juce::Component::SafePointer<BrainFogEditor> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [safe] (const juce::FileChooser& fc)
                              {
                                  if (safe == nullptr) return;
                                  auto f = fc.getResult();
                                  if (f.existsAsFile()) safe->proc.loadUserPreset (f);
                              });
    };
    addAndMakeVisible (saveBtn);
    addAndMakeVisible (loadBtn);

    resized();
    startTimerHz (30);
}

BrainFogEditor::~BrainFogEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void BrainFogEditor::buildGrunge (int w, int h)
{
    grunge = juce::Image (juce::Image::ARGB, w, h, true);
    juce::Graphics g (grunge);
    juce::Random rnd (369);
    for (int i = 0; i < 9000; ++i)                       // grano / polvo
    {
        g.setColour (juce::Colours::white.withAlpha (rnd.nextFloat() * 0.05f));
        g.fillRect (rnd.nextInt (w), rnd.nextInt (h), 1 + rnd.nextInt (2), 1);
    }
    for (int i = 0; i < 55; ++i)                         // aranazos
    {
        g.setColour (juce::Colours::white.withAlpha (0.03f + rnd.nextFloat() * 0.05f));
        const float x = rnd.nextFloat() * (float) w, y = rnd.nextFloat() * (float) h, len = 20.0f + rnd.nextFloat() * 140.0f;
        const float a = rnd.nextFloat() * 6.2832f;
        g.drawLine (x, y, x + std::cos (a) * len, y + std::sin (a) * len, 0.6f);
    }
    for (int i = 0; i < 18; ++i)                         // manchas de oxido
    {
        g.setColour (bfcol::blood.withAlpha (0.015f + rnd.nextFloat() * 0.03f));
        const float r = 20.0f + rnd.nextFloat() * 70.0f;
        g.fillEllipse (rnd.nextFloat() * (float) w - r, rnd.nextFloat() * (float) h - r, r * 2.0f, r * 2.0f);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, (float) w * 0.5f, (float) h * 0.5f,
                                             juce::Colours::black.withAlpha (0.55f), 0.0f, 0.0f, true));
    g.fillAll();                                         // vineta
}

void BrainFogEditor::paint (juce::Graphics& g)
{
    using namespace bfcol;
    g.fillAll (bg);
    g.drawImageAt (grunge, 0, 0);

    for (auto& s : sections)
    {
        auto r = s.area.toFloat();
        g.setColour (panel.withAlpha (0.88f));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (edge);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
        g.setColour (blood);
        g.fillRect (r.getX() + 8.0f, r.getY() + 20.0f, 22.0f, 1.5f);
        g.setColour (bone);
        g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
        g.drawText (s.title, (int) r.getX() + 8, (int) r.getY() + 4, (int) r.getWidth() - 16, 14, juce::Justification::left);
    }

    g.setColour (bone);
    g.setFont (juce::Font (juce::FontOptions (36.0f).withStyle ("Bold")));
    g.drawText ("d-BrainFog", 18, 8, 300, 44, juce::Justification::left);
    g.setColour (blood);
    g.fillRect (20, 54, 150, 2);
    g.setColour (dim);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("DHA369  |  v1.0  |  cold analog synth", 20, 58, 300, 14, juce::Justification::left);

    // medidor de salida
    auto m = meterArea().toFloat();
    g.setColour (juce::Colour (0xff060606)); g.fillRoundedRectangle (m, 2.0f);
    const float db = juce::Decibels::gainToDecibels (meterDisp, -60.0f);
    const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
    g.setColour (db > -3.0f ? blood : bone.withAlpha (0.8f));
    g.fillRoundedRectangle (m.withWidth (m.getWidth() * frac), 2.0f);
    g.setColour (edge); g.drawRoundedRectangle (m, 2.0f, 1.0f);
    g.setColour (dim); g.setFont (juce::Font (juce::FontOptions (9.5f)));
    g.drawText ("OUT dB", (int) m.getX(), (int) m.getBottom() + 1, 60, 12, juce::Justification::left);
}

void BrainFogEditor::placeControl (Ctl& c, juce::Rectangle<int> cell)
{
    if (c.toggle)
    {
        c.toggle->setBounds (cell.withSizeKeepingCentre (cell.getWidth() - 4, 26));
        return;
    }
    auto lab = cell.removeFromBottom (14);
    if (c.label) c.label->setBounds (lab);
    if (c.slider) c.slider->setBounds (cell);
    if (c.combo)  c.combo->setBounds (cell.withSizeKeepingCentre (cell.getWidth() - 6, 22));
}

void BrainFogEditor::resized()
{
    // cabecera
    presetBox.setBounds (330, 14, 190, 26);
    saveBtn.setBounds (528, 14, 54, 26);
    loadBtn.setBounds (586, 14, 54, 26);
    presetLabel.setBounds (330, 44, 300, 16);
    {
        auto& c = ctls[(size_t) bf::P_VOLUME];
        if (c.slider) c.slider->setBounds (kW - 330, 4, 70, 58);
        if (c.label)  c.label->setBounds (kW - 330, 60, 70, 14);
    }

    sections[0].area = { 10, 84, 980, 134 };    // macros
    sections[1].area = { 10, 226, 430, 202 };   // osciladores
    sections[2].area = { 450, 226, 220, 202 };  // filtro
    sections[6].area = { 680, 226, 310, 202 };  // LFO
    sections[4].area = { 10, 436, 280, 130 };   // amp env
    sections[5].area = { 300, 436, 280, 130 };  // filter env
    sections[3].area = { 590, 436, 400, 130 };  // voice
    sections[7].area = { 10, 574, 980, 118 };   // efectos

    for (auto& s : sections)
    {
        auto inner = s.area.reduced (8).withTrimmedTop (18);
        const int cols = juce::jmax (1, inner.getWidth() / s.cellW);
        for (size_t k = 0; k < s.ids.size(); ++k)
        {
            const int col = (int) k % cols, row = (int) k / cols;
            placeControl (ctls[(size_t) s.ids[k]],
                          { inner.getX() + col * s.cellW, inner.getY() + row * s.cellH, s.cellW, s.cellH });
        }
    }
}

void BrainFogEditor::timerCallback()
{
    const float pk = proc.meterPeak.exchange (0.0f);
    meterDisp = juce::jmax (pk, meterDisp * 0.9f);

    if (proc.getCurrentProgram() != lastProgram)
    {
        lastProgram = proc.getCurrentProgram();
        presetBox.setSelectedItemIndex (lastProgram, juce::dontSendNotification);
    }
    if (proc.currentPresetName != lastName)
    {
        lastName = proc.currentPresetName;
        presetLabel.setText ("Preset: " + lastName, juce::dontSendNotification);
    }
    repaint (meterArea().expanded (0, 14));
}
