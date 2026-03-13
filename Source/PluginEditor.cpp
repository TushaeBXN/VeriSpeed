#include "PluginEditor.h"

//==============================================================================
// DarkKnobLF – dark rubberized rotary knob with white indicator line
void VeriSpeedEditor::DarkKnobLF::drawRotarySlider (
    juce::Graphics& g,
    int x, int y, int w, int h,
    float sliderPos,
    float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider& /*slider*/)
{
    const float cx     = static_cast<float> (x) + static_cast<float> (w) * 0.5f;
    const float cy     = static_cast<float> (y) + static_cast<float> (h) * 0.5f;
    const float radius = juce::jmin (static_cast<float> (w), static_cast<float> (h)) * 0.42f;
    const float angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Outer shadow ring
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (cx - radius - 3, cy - radius - 3,
                   (radius + 3) * 2, (radius + 3) * 2);

    // Knob body gradient (dark rubber)
    juce::ColourGradient bodyGrad (
        juce::Colour (0xff484848), cx - radius * 0.4f, cy - radius * 0.4f,
        juce::Colour (0xff1a1a1a), cx + radius * 0.4f, cy + radius * 0.4f,
        true);
    g.setGradientFill (bodyGrad);
    g.fillEllipse (cx - radius, cy - radius, radius * 2, radius * 2);

    // Subtle rim highlight
    g.setColour (juce::Colour (0xff606060));
    g.drawEllipse (cx - radius, cy - radius, radius * 2, radius * 2, 1.5f);

    // Arc track (dark groove)
    {
        juce::Path track;
        track.addArc (cx - radius * 0.78f, cy - radius * 0.78f,
                      radius * 1.56f, radius * 1.56f,
                      rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff303030));
        g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // Filled arc (value indicator)
    {
        juce::Path filledArc;
        filledArc.addArc (cx - radius * 0.78f, cy - radius * 0.78f,
                          radius * 1.56f, radius * 1.56f,
                          rotaryStartAngle, angle, true);
        g.setColour (juce::Colour (0xffc8960c).withAlpha (0.7f));
        g.strokePath (filledArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
    }

    // White indicator line
    const float lineLen = radius * 0.65f;
    g.setColour (juce::Colours::white);
    g.drawLine (cx, cy,
                cx + std::cos (angle) * lineLen,
                cy + std::sin (angle) * lineLen,
                2.0f);

    // Centre dot
    g.setColour (juce::Colour (0xff555555));
    g.fillEllipse (cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
}

//==============================================================================
VeriSpeedEditor::VeriSpeedEditor (VeriSpeedAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (400, 300);

    // ── Look and Feel ─────────────────────────────────────────────────────────
    leftKnob.setLookAndFeel  (&darkKnobLF);
    rightKnob.setLookAndFeel (&darkKnobLF);

    // ── Knob configuration ────────────────────────────────────────────────────
    // Both knobs control the same semitone_shift parameter.
    // Left knob turns it down (−1 step); right knob turns it up (+1 step).
    // We use a single APVTS attachment on leftKnob (full range) and
    // make rightKnob a mirrored view of the same parameter.

    leftKnob.setRange  (-24.0, 24.0, 0.01);
    rightKnob.setRange (-24.0, 24.0, 0.01);
    leftKnob.setSkewFactor  (1.0);
    rightKnob.setSkewFactor (1.0);

    // APVTS attachment for the main semitone slider
    leftKnobAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ParamID::semitoneShift, leftKnob);

    // rightKnob mirrors leftKnob via a lambda listener
    rightKnob.onValueChange = [this] {
        leftKnob.setValue (rightKnob.getValue(), juce::sendNotificationSync);
    };
    leftKnob.onValueChange = [this] {
        // Sync right knob without recursion
        rightKnob.setValue (leftKnob.getValue(), juce::dontSendNotification);
        // Update semitone readout
        const float st = static_cast<float> (leftKnob.getValue());
        const juce::String sign = st > 0.0f ? "+" : "";
        semitoneLabel.setText (sign + juce::String (st, 2) + " st",
                               juce::dontSendNotification);
        vuMeter.setValue (st);
    };

    // ── Tempo button ──────────────────────────────────────────────────────────
    tempoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, ParamID::tempoOn, tempoButton);

    // ── Reset button ──────────────────────────────────────────────────────────
    resetButton.onClick = [this] {
        processor.apvts.getParameter (ParamID::semitoneShift)->setValueNotifyingHost (
            processor.apvts.getParameter (ParamID::semitoneShift)->getDefaultValue());
    };

    // ── Semitone readout ──────────────────────────────────────────────────────
    semitoneLabel.setText ("0.00 st", juce::dontSendNotification);
    semitoneLabel.setJustificationType (juce::Justification::centred);
    semitoneLabel.setFont (juce::Font (22.0f, juce::Font::bold));
    semitoneLabel.setColour (juce::Label::textColourId, juce::Colours::white);

    // ── Exclude field ─────────────────────────────────────────────────────────
    excludeLabel.setText ("exclude:", juce::dontSendNotification);
    excludeLabel.setFont (juce::Font (11.0f));
    excludeLabel.setColour (juce::Label::textColourId, juce::Colour (0xff999999));
    excludeLabel.setJustificationType (juce::Justification::centredRight);

    excludeEditor.setMultiLine (false);
    excludeEditor.setReturnKeyStartsNewLine (false);
    excludeEditor.setText (
        processor.apvts.state.getProperty ("excludeSymbol", "@").toString());
    excludeEditor.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
    excludeEditor.setColour (juce::TextEditor::backgroundColourId,  juce::Colour (0xff111111));
    excludeEditor.setColour (juce::TextEditor::textColourId,        juce::Colour (0xffe0e0e0));
    excludeEditor.setColour (juce::TextEditor::outlineColourId,     juce::Colour (0xff444444));
    excludeEditor.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (0xffc8960c));
    excludeEditor.onTextChange = [this] {
        processor.apvts.state.setProperty ("excludeSymbol",
                                            excludeEditor.getText(), nullptr);
    };

    // ── Author label ──────────────────────────────────────────────────────────
    authorLabel.setText ("by Brian Tushae Thomas", juce::dontSendNotification);
    authorLabel.setFont (juce::Font (10.0f, juce::Font::italic));
    authorLabel.setColour (juce::Label::textColourId, juce::Colour (0xff666666));
    authorLabel.setJustificationType (juce::Justification::centredRight);

    // ── Add all children ──────────────────────────────────────────────────────
    addAndMakeVisible (tempoButton);
    addAndMakeVisible (leftKnob);
    addAndMakeVisible (vuMeter);
    addAndMakeVisible (rightKnob);
    addAndMakeVisible (semitoneLabel);
    addAndMakeVisible (resetButton);
    addAndMakeVisible (excludeLabel);
    addAndMakeVisible (excludeEditor);
    addAndMakeVisible (authorLabel);

    // ── Timer for VU meter + live semitone sync ───────────────────────────────
    startTimerHz (30);
}

VeriSpeedEditor::~VeriSpeedEditor()
{
    stopTimer();
    leftKnob.setLookAndFeel  (nullptr);
    rightKnob.setLookAndFeel (nullptr);
}

//==============================================================================
void VeriSpeedEditor::timerCallback()
{
    const float st = processor.getCurrentSemitones();
    vuMeter.setValue (st);

    const juce::String sign = st > 0.0f ? "+" : "";
    semitoneLabel.setText (sign + juce::String (st, 2) + " st",
                           juce::dontSendNotification);
}

//==============================================================================
void VeriSpeedEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // ── Background ────────────────────────────────────────────────────────────
    juce::ColourGradient bg (juce::Colour (0xff2f2f2f), 0, 0,
                              juce::Colour (0xff1e1e1e), 0, static_cast<float> (getHeight()),
                              false);
    g.setGradientFill (bg);
    g.fillAll();

    // Subtle inner border
    g.setColour (juce::Colour (0xff404040));
    g.drawRect (bounds.reduced (1.5f), 1.0f);

    // ── Title ─────────────────────────────────────────────────────────────────
    g.setFont (juce::Font (16.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xffc8c8c8));
    g.drawText ("Varispeed  1.1",
                0, 8, getWidth(), 20,
                juce::Justification::centred, false);

    // ── Knob labels ───────────────────────────────────────────────────────────
    g.setFont (juce::Font (10.0f, juce::Font::plain));
    g.setColour (juce::Colour (0xff888888));
    g.drawText ("-1 semitone", 10, 52, 110, 14, juce::Justification::centred, false);
    g.drawText ("+1 semitone", getWidth() - 120, 52, 110, 14, juce::Justification::centred, false);
}

//==============================================================================
void VeriSpeedEditor::resized()
{
    const int W = getWidth();
    const int H = getHeight();

    // ── Tempo button – top-left ───────────────────────────────────────────────
    tempoButton.setBounds (10, 8, 90, 22);

    // Author – bottom-right
    authorLabel.setBounds (W - 180, H - 18, 175, 14);

    // ── Three-column row: left knob | vu meter | right knob ───────────────────
    constexpr int rowTop    = 66;
    constexpr int rowH      = 100;
    constexpr int knobW     = 80;
    constexpr int knobH     = 80;
    constexpr int vuW       = 120;
    constexpr int vuH       = 100;

    const int totalRowW     = knobW + vuW + knobW + 20; // padding
    const int rowLeft       = (W - totalRowW) / 2;

    leftKnob.setBounds  (rowLeft,                          rowTop + (rowH - knobH) / 2, knobW, knobH);
    vuMeter.setBounds   (rowLeft + knobW + 10,             rowTop, vuW, vuH);
    rightKnob.setBounds (rowLeft + knobW + 10 + vuW + 10,  rowTop + (rowH - knobH) / 2, knobW, knobH);

    // ── Semitone readout – centred below knob row ─────────────────────────────
    semitoneLabel.setBounds ((W - 160) / 2, rowTop + rowH + 4, 160, 28);

    // ── Reset button ──────────────────────────────────────────────────────────
    resetButton.setBounds ((W - 120) / 2, rowTop + rowH + 36, 120, 24);

    // ── Exclude row ───────────────────────────────────────────────────────────
    const int exY = rowTop + rowH + 66;
    excludeLabel.setBounds  (10,        exY, 70,  22);
    excludeEditor.setBounds (84,        exY, 60,  22);
}
