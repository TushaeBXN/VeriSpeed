#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "VUMeter.h"
#include "PillButton.h"

//==============================================================================
class VeriSpeedEditor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit VeriSpeedEditor (VeriSpeedAudioProcessor&);
    ~VeriSpeedEditor() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    VeriSpeedAudioProcessor& processor;

    //==========================================================================
    // Shared knob look-and-feel
    struct DarkKnobLF : public juce::LookAndFeel_V4
    {
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                               float sliderPosProportional,
                               float rotaryStartAngle, float rotaryEndAngle,
                               juce::Slider&) override;
    } darkKnobLF;

    // Shae Color knob gets a special gold-tinted LF
    struct ColorKnobLF : public juce::LookAndFeel_V4
    {
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                               float sliderPosProportional,
                               float rotaryStartAngle, float rotaryEndAngle,
                               juce::Slider&) override;
    } colorKnobLF;

    //==========================================================================
    // Controls

    // Tempo toggle
    PillButton tempoButton { "Tempo On", PillButton::Style::Gold };

    // Semitone step buttons (replaces two knobs)
    PillButton semitoneDownBtn { "-1 st", PillButton::Style::Steel };
    PillButton semitoneUpBtn   { "+1 st", PillButton::Style::Steel };

    // Semitone fine-tune knob (hidden / ctrl-drag style, small)
    juce::Slider semitoneKnob { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };

    // Shae Color knob
    juce::Slider shaeColorKnob { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Label  shaeColorLabel;
    juce::Label  shaeColorValueLabel;

    // VU meter
    VUMeter vuMeter;

    // Semitone readout
    juce::Label semitoneLabel;

    // Reset
    PillButton resetButton { "RESET", PillButton::Style::Steel };

    // Exclude
    juce::Label      excludeLabel;
    juce::TextEditor excludeEditor;

    // Author
    juce::Label authorLabel;

    //==========================================================================
    // APVTS attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> semitoneAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> shaeColorAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tempoAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VeriSpeedEditor)
};
