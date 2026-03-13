#include "VUMeter.h"

//==============================================================================
VUMeter::VUMeter()
{
    startTimerHz (30); // 30 fps needle animation
}

VUMeter::~VUMeter()
{
    stopTimer();
}

//==============================================================================
void VUMeter::setValue (float semitones)
{
    targetValue.store (juce::jlimit (-kRange, kRange, semitones));
}

//==============================================================================
void VUMeter::timerCallback()
{
    const float target = targetValue.load();
    // Exponential smoothing (ballistic simulation)
    displayValue += (target - displayValue) * kSmoothCoef;

    if (std::abs (displayValue - target) > 0.001f)
        repaint();
    else if (std::abs (displayValue - target) > 0.0001f)
    {
        displayValue = target;
        repaint();
    }
}

//==============================================================================
void VUMeter::resized()
{
    const auto b = getLocalBounds().toFloat().reduced (4.0f);
    // Place centre at bottom-centre of the bounding box
    centre = { b.getCentreX(), b.getBottom() - b.getHeight() * 0.08f };
    radius = juce::jmin (b.getWidth() * 0.48f, b.getHeight() * 0.92f);
}

//==============================================================================
juce::Point<float> VUMeter::needleTip (float semitones) const
{
    // Map –range..+range → –kArcDeg/2 .. +kArcDeg/2 (0° = straight up = 270° in juce)
    const float normalised = juce::jlimit (-1.0f, 1.0f, semitones / kRange);
    const float angleDeg   = normalised * (kArcDeg * 0.5f);  // –60 .. +60°
    // JUCE angles: 0° = right, CCW positive in screen space (y-down)
    // We want 0 semitones to point straight up → base angle = -90° (i.e. 270°)
    const float angleRad   = juce::degreesToRadians (-90.0f + angleDeg);

    return centre.translated (std::cos (angleRad) * radius * 0.88f,
                               std::sin (angleRad) * radius * 0.88f);
}

//==============================================================================
void VUMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (2.0f);

    // ── Face background ───────────────────────────────────────────────────────
    g.setColour (juce::Colour (0xfff5f0dc)); // cream/beige
    g.fillEllipse (bounds);

    g.setColour (juce::Colour (0xff888878));
    g.drawEllipse (bounds, 1.5f);

    // ── Red zone arcs (beyond ±4 semitones) ──────────────────────────────────
    {
        juce::Path redPath;
        // Convert semitone ±4 and ±5 to angles
        auto semToAngle = [&] (float st) -> float {
            const float n = st / kRange;
            return juce::degreesToRadians (-90.0f + n * kArcDeg * 0.5f);
        };

        const float r1 = radius * 0.60f;
        const float r2 = radius * 0.85f;

        // Right red zone (+4 to +5)
        {
            const float a1 = semToAngle (kRedZone);
            const float a2 = semToAngle (kRange);
            juce::Path arc;
            arc.addArc (centre.x - r2, centre.y - r2, r2 * 2, r2 * 2, a1, a2, true);
            arc.addArc (centre.x - r1, centre.y - r1, r1 * 2, r1 * 2, a2, a1, false);
            arc.closeSubPath();
            g.setColour (juce::Colour (0xffcc2200).withAlpha (0.55f));
            g.fillPath (arc);
        }
        // Left red zone (−5 to −4)
        {
            const float a1 = semToAngle (-kRange);
            const float a2 = semToAngle (-kRedZone);
            juce::Path arc;
            arc.addArc (centre.x - r2, centre.y - r2, r2 * 2, r2 * 2, a1, a2, true);
            arc.addArc (centre.x - r1, centre.y - r1, r1 * 2, r1 * 2, a2, a1, false);
            arc.closeSubPath();
            g.setColour (juce::Colour (0xffcc2200).withAlpha (0.55f));
            g.fillPath (arc);
        }
    }

    // ── Scale markings ────────────────────────────────────────────────────────
    {
        g.setColour (juce::Colour (0xff222222));
        const float tickOuter = radius * 0.87f;
        const float tickInner = radius * 0.75f;

        for (int st = -5; st <= 5; ++st)
        {
            const float n        = static_cast<float> (st) / kRange;
            const float angleRad = juce::degreesToRadians (-90.0f + n * kArcDeg * 0.5f);
            const float cosA     = std::cos (angleRad);
            const float sinA     = std::sin (angleRad);

            const bool isMajor = (st % 5 == 0) || st == 0;
            const float inner  = isMajor ? tickInner : (tickOuter + tickInner) * 0.5f;
            const float thick  = isMajor ? 1.8f : 1.0f;

            g.drawLine (centre.x + cosA * inner,  centre.y + sinA * inner,
                        centre.x + cosA * tickOuter, centre.y + sinA * tickOuter,
                        thick);

            // Label major ticks
            if (isMajor)
            {
                const float lx = centre.x + cosA * (tickInner - 11.0f);
                const float ly = centre.y + sinA * (tickInner - 11.0f);
                g.setFont (juce::Font (9.0f, juce::Font::plain));
                juce::String label = (st > 0 ? "+" : "") + juce::String (st);
                g.drawText (label,
                            static_cast<int> (lx - 10), static_cast<int> (ly - 6),
                            20, 12,
                            juce::Justification::centred, false);
            }
        }
    }

    // ── Needle ────────────────────────────────────────────────────────────────
    {
        const auto tip = needleTip (displayValue);

        // Shadow
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawLine (centre.x + 1, centre.y + 1, tip.x + 1, tip.y + 1, 1.5f);

        // Needle body
        g.setColour (juce::Colour (0xff111111));
        g.drawLine (centre.x, centre.y, tip.x, tip.y, 1.8f);

        // Pivot dot
        g.setColour (juce::Colour (0xff333333));
        g.fillEllipse (centre.x - 4.0f, centre.y - 4.0f, 8.0f, 8.0f);
        g.setColour (juce::Colour (0xff888888));
        g.fillEllipse (centre.x - 2.5f, centre.y - 2.5f, 5.0f, 5.0f);
    }

    // ── Centre label "0" ─────────────────────────────────────────────────────
    {
        g.setColour (juce::Colour (0xff333333));
        g.setFont (juce::Font (9.0f, juce::Font::bold));
        g.drawText ("0",
                    static_cast<int> (centre.x - 8),
                    static_cast<int> (centre.y - radius * 0.45f - 6),
                    16, 12,
                    juce::Justification::centred, false);
    }
}
