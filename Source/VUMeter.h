#pragma once
#include <JuceHeader.h>

/**
 * VUMeter
 *
 * Analog-style VU meter with a cream/beige face, black needle, and red zones
 * beyond ±4 semitones.  Range is ±5 semitones drawn as an arc.
 *
 * Call setValue() from the audio thread (or a timer) to update the needle.
 */
class VUMeter : public juce::Component,
                private juce::Timer
{
public:
    VUMeter();
    ~VUMeter() override;

    /** Thread-safe – call from any thread. */
    void setValue (float semitones);

    //==========================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Smoothed display value
    std::atomic<float> targetValue { 0.0f };
    float displayValue = 0.0f;

    // Geometry (computed in resized)
    juce::Point<float> centre;
    float radius = 0.0f;

    // Helpers
    juce::Point<float> needleTip (float semitones) const;

    static constexpr float kRange      = 5.0f;   // ±5 semitones full scale
    static constexpr float kRedZone    = 4.0f;   // red beyond ±4
    static constexpr float kArcDeg     = 120.0f; // total arc in degrees
    static constexpr float kSmoothCoef = 0.15f;  // needle ballistics

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VUMeter)
};
