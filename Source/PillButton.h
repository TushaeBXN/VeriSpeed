#pragma once
#include <JuceHeader.h>

/**
 * PillButton
 *
 * A rounded pill-shaped toggle/momentary button with two visual presets:
 *   Style::Gold  – amber/gold (#c8960c) with glow when active (Tempo_on)
 *   Style::Steel – brushed steel gradient (RESET button)
 */
class PillButton : public juce::TextButton
{
public:
    enum class Style { Gold, Steel };

    explicit PillButton (const juce::String& label, Style style = Style::Steel);
    ~PillButton() override = default;

    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;

private:
    Style buttonStyle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PillButton)
};
