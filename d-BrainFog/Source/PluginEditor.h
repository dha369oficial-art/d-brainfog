#pragma once
#include "PluginProcessor.h"

// Estetica: negro / gris carbon, rojo sangre, blanco envejecido, desgaste grunge
namespace bfcol
{
    const juce::Colour bg    { 0xff0a0a0a };
    const juce::Colour panel { 0xff1a1a1c };
    const juce::Colour edge  { 0xff3a3a3d };
    const juce::Colour blood { 0xffb3121c };
    const juce::Colour bone  { 0xffd8cfbb };
    const juce::Colour dim   { 0xff8a8578 };
}

class BrainFogLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BrainFogLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

class BrainFogEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BrainFogEditor (BrainFogProcessor&);
    ~BrainFogEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Ctl
    {
        std::unique_ptr<juce::Slider>       slider;
        std::unique_ptr<juce::ComboBox>     combo;
        std::unique_ptr<juce::ToggleButton> toggle;
        std::unique_ptr<juce::Label>        label;
        std::unique_ptr<SliderAtt> sa;      // los attachments se declaran despues: se destruyen antes que los controles
        std::unique_ptr<ComboAtt>  ca;
        std::unique_ptr<ButtonAtt> ba;
    };

    struct Section
    {
        juce::String title;
        std::vector<int> ids;
        juce::Rectangle<int> area;
        int cellW = 66, cellH = 84;
    };

    void timerCallback() override;
    void placeControl (Ctl&, juce::Rectangle<int> cell);
    void buildGrunge (int w, int h);
    juce::Rectangle<int> meterArea() const { return { getWidth() - 250, 40, 230, 14 }; }

    BrainFogProcessor& proc;
    BrainFogLookAndFeel laf;
    std::array<Ctl, bf::P_COUNT> ctls;
    std::vector<Section> sections;

    juce::ComboBox presetBox;
    juce::TextButton saveBtn { "Save" }, loadBtn { "Load" };
    juce::Label presetLabel;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Image grunge;
    float meterDisp = 0.0f;
    int lastProgram = -1;
    juce::String lastName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrainFogEditor)
};
