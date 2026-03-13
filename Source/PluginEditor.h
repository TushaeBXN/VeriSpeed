#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "VUMeter.h"
#include "PillButton.h"

//==============================================================================
/**
 * VeriSpeedEditor
 *
 * UI layout:
 *
 *   ┌──────────────────────────────────────┐
 *   │  [Tempo_on]        Varispeed  1.1   │
 *   │                                      │
 *   │  -1 semitone      +1 semitone        │
 *   │  [KNOB L]  [VU METER]  [KNOB R]     │
 *   │             -5  0  +5               │
 *   │                  0                   │
 *   │            [ RESET ]                 │
 *   │  exclude:  [ @ ]                     │
 *   │       by Brian Tushae Thomas         │
 *   └──────────────────────────────────────┘
 */
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
    void layoutComponents();

    VeriSpeedAudioProcessor& processor;

    //==========================================================================
    // Knob look-and-feel (dark rubberized)
    struct DarkKnobLF : public juce::LookAndFeel_V4
    {
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                               float sliderPosProportional,
                               float rotaryStartAngle, float rotaryEndAngle,
                               juce::Slider&) override;
    } darkKnobLF;

    //==========================================================================
    // Controls
    PillButton tempoButton  { "Tempo On", PillButton::Style::Gold  };
    PillButton resetButton  { "RESET",    PillButton::Style::Steel };

    // Left knob: −1 semitone increment
    juce::Slider leftKnob  { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    // Right knob: +1 semitone increment
    juce::Slider rightKnob { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };

    // Semitone readout
    juce::Label semitoneLabel;

    // VU meter
    VUMeter vuMeter;

    // Exclude symbol input
    juce::Label     excludeLabel;
    juce::TextEditor excludeEditor;

    // Author label
    juce::Label authorLabel;

    //==========================================================================
    // APVTS attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> leftKnobAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rightKnobAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tempoAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VeriSpeedEditor)
};
