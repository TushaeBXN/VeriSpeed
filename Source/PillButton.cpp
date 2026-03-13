#include "PillButton.h"

//==============================================================================
PillButton::PillButton (const juce::String& label, Style style)
    : juce::TextButton (label), buttonStyle (style)
{
    setClickingTogglesState (style == Style::Gold);
}

//==============================================================================
void PillButton::paintButton (juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    const auto bounds = getLocalBounds().toFloat().reduced (1.5f);
    const float corner = bounds.getHeight() * 0.5f; // full pill rounding
    const bool  active = getToggleState() || isButtonDown;

    if (buttonStyle == Style::Gold)
    {
        // ── Gold / amber pill ─────────────────────────────────────────────
        const juce::Colour baseColour =
            active ? juce::Colour (0xffc8960c) : juce::Colour (0xff6b5008);
        const juce::Colour topColour  = baseColour.brighter (0.3f);
        const juce::Colour botColour  = baseColour.darker  (0.3f);

        juce::ColourGradient grad (topColour, bounds.getX(), bounds.getY(),
                                   botColour, bounds.getX(), bounds.getBottom(),
                                   false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, corner);

        // Glow halo when active
        if (active)
        {
            const auto glowBounds = bounds.expanded (3.0f);
            g.setColour (juce::Colour (0xffc8960c).withAlpha (0.35f));
            g.fillRoundedRectangle (glowBounds, corner + 3.0f);
        }

        // Hover highlight
        if (isMouseOver && !active)
        {
            g.setColour (juce::Colours::white.withAlpha (0.07f));
            g.fillRoundedRectangle (bounds, corner);
        }

        // Border
        g.setColour (active ? juce::Colour (0xffffe082) : juce::Colour (0xff4a3800));
        g.drawRoundedRectangle (bounds, corner, 1.2f);

        // Label
        g.setColour (active ? juce::Colours::white : juce::Colour (0xffe0c060));
        g.setFont (juce::Font (12.0f, juce::Font::bold));
        g.drawText (getButtonText(), bounds.toNearestInt(), juce::Justification::centred, false);
    }
    else // Steel
    {
        // ── Brushed steel pill ────────────────────────────────────────────
        const juce::Colour top = isButtonDown ? juce::Colour (0xff606060)
                                              : juce::Colour (0xff9a9a9a);
        const juce::Colour bot = isButtonDown ? juce::Colour (0xff383838)
                                              : juce::Colour (0xff555555);

        juce::ColourGradient grad (top, bounds.getX(), bounds.getY(),
                                   bot, bounds.getX(), bounds.getBottom(),
                                   false);

        // Brushed-steel banding
        grad.addColour (0.45, top.interpolatedWith (bot, 0.4f).brighter (0.1f));
        grad.addColour (0.50, top.interpolatedWith (bot, 0.5f));
        grad.addColour (0.55, top.interpolatedWith (bot, 0.6f).darker  (0.1f));

        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, corner);

        // Specular top edge
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        const auto topStrip = bounds.withHeight (bounds.getHeight() * 0.3f);
        g.fillRoundedRectangle (topStrip, corner);

        // Hover
        if (isMouseOver)
        {
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.fillRoundedRectangle (bounds, corner);
        }

        // Border
        g.setColour (juce::Colour (0xff222222));
        g.drawRoundedRectangle (bounds, corner, 1.2f);

        // Label
        g.setColour (isButtonDown ? juce::Colour (0xffa0a0a0) : juce::Colours::white);
        g.setFont (juce::Font (12.5f, juce::Font::bold));
        g.drawText (getButtonText(), bounds.toNearestInt(), juce::Justification::centred, false);
    }
}
