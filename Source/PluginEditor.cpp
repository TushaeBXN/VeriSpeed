#include "PluginEditor.h"

//==============================================================================
// Dark knob (semitone)
void VeriSpeedEditor::DarkKnobLF::drawRotarySlider (
    juce::Graphics& g, int x, int y, int w, int h,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider&)
{
    const float cx     = x + w * 0.5f;
    const float cy     = y + h * 0.5f;
    const float radius = juce::jmin (w, h) * 0.42f;
    const float angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (cx - radius - 3, cy - radius - 3, (radius + 3) * 2, (radius + 3) * 2);

    juce::ColourGradient body (juce::Colour (0xff484848), cx - radius * 0.4f, cy - radius * 0.4f,
                                juce::Colour (0xff1a1a1a), cx + radius * 0.4f, cy + radius * 0.4f, true);
    g.setGradientFill (body);
    g.fillEllipse (cx - radius, cy - radius, radius * 2, radius * 2);

    g.setColour (juce::Colour (0xff606060));
    g.drawEllipse (cx - radius, cy - radius, radius * 2, radius * 2, 1.5f);

    {
        juce::Path arc;
        arc.addArc (cx - radius * 0.78f, cy - radius * 0.78f, radius * 1.56f, radius * 1.56f,
                    rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff303030));
        g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    {
        juce::Path filled;
        filled.addArc (cx - radius * 0.78f, cy - radius * 0.78f, radius * 1.56f, radius * 1.56f,
                       rotaryStartAngle, angle, true);
        g.setColour (juce::Colour (0xffc8960c).withAlpha (0.7f));
        g.strokePath (filled, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour (juce::Colours::white);
    g.drawLine (cx, cy, cx + std::cos (angle) * radius * 0.65f, cy + std::sin (angle) * radius * 0.65f, 2.0f);
    g.setColour (juce::Colour (0xff555555));
    g.fillEllipse (cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
}

//==============================================================================
// Shae Color knob — gold/amber tinted, glows at high values
void VeriSpeedEditor::ColorKnobLF::drawRotarySlider (
    juce::Graphics& g, int x, int y, int w, int h,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider&)
{
    const float cx     = x + w * 0.5f;
    const float cy     = y + h * 0.5f;
    const float radius = juce::jmin (w, h) * 0.42f;
    const float angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Glow ring (grows with value)
    if (sliderPos > 0.05f)
    {
        const float glowR = radius + 4.0f + sliderPos * 6.0f;
        g.setColour (juce::Colour (0xffc8960c).withAlpha (sliderPos * 0.4f));
        g.fillEllipse (cx - glowR, cy - glowR, glowR * 2, glowR * 2);
    }

    // Shadow
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (cx - radius - 3, cy - radius - 3, (radius + 3) * 2, (radius + 3) * 2);

    // Body — dark with gold tint at high values
    const juce::Colour bodyTop = juce::Colour (0xff484848).interpolatedWith (juce::Colour (0xff4a3800), sliderPos * 0.6f);
    const juce::Colour bodyBot = juce::Colour (0xff1a1a1a).interpolatedWith (juce::Colour (0xff1e1400), sliderPos * 0.6f);
    juce::ColourGradient body (bodyTop, cx - radius * 0.4f, cy - radius * 0.4f,
                                bodyBot, cx + radius * 0.4f, cy + radius * 0.4f, true);
    g.setGradientFill (body);
    g.fillEllipse (cx - radius, cy - radius, radius * 2, radius * 2);

    // Rim
    g.setColour (juce::Colour (0xff606060).interpolatedWith (juce::Colour (0xffc8960c), sliderPos * 0.8f));
    g.drawEllipse (cx - radius, cy - radius, radius * 2, radius * 2, 1.8f);

    // Track
    {
        juce::Path arc;
        arc.addArc (cx - radius * 0.78f, cy - radius * 0.78f, radius * 1.56f, radius * 1.56f,
                    rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff2a2000));
        g.strokePath (arc, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    {
        juce::Path filled;
        filled.addArc (cx - radius * 0.78f, cy - radius * 0.78f, radius * 1.56f, radius * 1.56f,
                       rotaryStartAngle, angle, true);
        const juce::Colour arcColor = juce::Colour (0xffc8960c).interpolatedWith (juce::Colour (0xffff6600), sliderPos);
        g.setColour (arcColor);
        g.strokePath (filled, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Indicator line
    g.setColour (juce::Colours::white);
    g.drawLine (cx, cy, cx + std::cos (angle) * radius * 0.65f, cy + std::sin (angle) * radius * 0.65f, 2.2f);
    g.setColour (juce::Colour (0xffc8960c));
    g.fillEllipse (cx - 3.5f, cy - 3.5f, 7.0f, 7.0f);
}

//==============================================================================
VeriSpeedEditor::VeriSpeedEditor (VeriSpeedAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (440, 340);

    // ── Look & feel ───────────────────────────────────────────────────────────
    semitoneKnob.setLookAndFeel  (&darkKnobLF);
    shaeColorKnob.setLookAndFeel (&colorKnobLF);

    // ── Semitone knob (hidden fine-control, driven by buttons) ────────────────
    semitoneKnob.setRange (-24.0, 24.0, 0.01);
    semitoneAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ParamID::semitoneShift, semitoneKnob);

    semitoneKnob.onValueChange = [this] {
        const float st = static_cast<float> (semitoneKnob.getValue());
        const juce::String sign = st > 0.0f ? "+" : "";
        semitoneLabel.setText (sign + juce::String (st, 2) + " st", juce::dontSendNotification);
        vuMeter.setValue (st);
    };

    // ── Semitone step buttons ─────────────────────────────────────────────────
    semitoneDownBtn.setClickingTogglesState (false);
    semitoneUpBtn.setClickingTogglesState (false);

    semitoneDownBtn.onClick = [this] {
        const double cur = semitoneKnob.getValue();
        semitoneKnob.setValue (juce::jmax (-24.0, cur - 1.0), juce::sendNotificationSync);
    };
    semitoneUpBtn.onClick = [this] {
        const double cur = semitoneKnob.getValue();
        semitoneKnob.setValue (juce::jmin (24.0, cur + 1.0), juce::sendNotificationSync);
    };

    // ── Shae Color knob ───────────────────────────────────────────────────────
    shaeColorKnob.setRange (0.0, 10.0, 0.01);
    shaeColorAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ParamID::shaeColor, shaeColorKnob);

    shaeColorKnob.onValueChange = [this] {
        shaeColorValueLabel.setText (juce::String (shaeColorKnob.getValue(), 1),
                                     juce::dontSendNotification);
    };

    shaeColorLabel.setText ("Shae Color", juce::dontSendNotification);
    shaeColorLabel.setJustificationType (juce::Justification::centred);
    shaeColorLabel.setColour (juce::Label::textColourId, juce::Colour (0xffc8960c));

    shaeColorValueLabel.setText ("0.0", juce::dontSendNotification);
    shaeColorValueLabel.setJustificationType (juce::Justification::centred);
    shaeColorValueLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe0c060));

    // ── Tempo button ──────────────────────────────────────────────────────────
    tempoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, ParamID::tempoOn, tempoButton);

    // ── Reset ─────────────────────────────────────────────────────────────────
    resetButton.setClickingTogglesState (false);
    resetButton.onClick = [this] {
        processor.apvts.getParameter (ParamID::semitoneShift)->setValueNotifyingHost (
            processor.apvts.getParameter (ParamID::semitoneShift)->getDefaultValue());
    };

    // ── Semitone readout ──────────────────────────────────────────────────────
    semitoneLabel.setText ("0.00 st", juce::dontSendNotification);
    semitoneLabel.setJustificationType (juce::Justification::centred);
    semitoneLabel.setColour (juce::Label::textColourId, juce::Colours::white);

    // ── Exclude ───────────────────────────────────────────────────────────────
    excludeLabel.setText ("exclude:", juce::dontSendNotification);
    excludeLabel.setColour (juce::Label::textColourId, juce::Colour (0xff999999));
    excludeLabel.setJustificationType (juce::Justification::centredRight);

    excludeEditor.setMultiLine (false);
    excludeEditor.setText (processor.apvts.state.getProperty ("excludeSymbol", "@").toString());
    excludeEditor.setColour (juce::TextEditor::backgroundColourId,     juce::Colour (0xff111111));
    excludeEditor.setColour (juce::TextEditor::textColourId,           juce::Colour (0xffe0e0e0));
    excludeEditor.setColour (juce::TextEditor::outlineColourId,        juce::Colour (0xff444444));
    excludeEditor.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (0xffc8960c));
    excludeEditor.onTextChange = [this] {
        processor.apvts.state.setProperty ("excludeSymbol", excludeEditor.getText(), nullptr);
    };

    // ── Author ────────────────────────────────────────────────────────────────
    authorLabel.setText ("by Brian Tushae Thomas", juce::dontSendNotification);
    authorLabel.setColour (juce::Label::textColourId, juce::Colour (0xff666666));
    authorLabel.setJustificationType (juce::Justification::centredRight);

    // ── Add children ──────────────────────────────────────────────────────────
    addAndMakeVisible (tempoButton);
    addAndMakeVisible (semitoneDownBtn);
    addAndMakeVisible (semitoneUpBtn);
    addChildComponent (semitoneKnob); // hidden — driven by buttons
    addAndMakeVisible (vuMeter);
    addAndMakeVisible (semitoneLabel);
    addAndMakeVisible (shaeColorKnob);
    addAndMakeVisible (shaeColorLabel);
    addAndMakeVisible (shaeColorValueLabel);
    addAndMakeVisible (resetButton);
    addAndMakeVisible (excludeLabel);
    addAndMakeVisible (excludeEditor);
    addAndMakeVisible (authorLabel);

    startTimerHz (30);
}

VeriSpeedEditor::~VeriSpeedEditor()
{
    stopTimer();
    semitoneKnob.setLookAndFeel  (nullptr);
    shaeColorKnob.setLookAndFeel (nullptr);
}

//==============================================================================
void VeriSpeedEditor::timerCallback()
{
    const float st = processor.getCurrentSemitones();
    vuMeter.setValue (st);
    const juce::String sign = st > 0.0f ? "+" : "";
    semitoneLabel.setText (sign + juce::String (st, 2) + " st", juce::dontSendNotification);

    const float color = static_cast<float> (shaeColorKnob.getValue());
    shaeColorValueLabel.setText (juce::String (color, 1), juce::dontSendNotification);
}

//==============================================================================
void VeriSpeedEditor::paint (juce::Graphics& g)
{
    const int W = getWidth();
    const int H = getHeight();

    // Background
    juce::ColourGradient bg (juce::Colour (0xff2f2f2f), 0, 0,
                              juce::Colour (0xff1a1a1a), 0, static_cast<float> (H), false);
    g.setGradientFill (bg);
    g.fillAll();

    g.setColour (juce::Colour (0xff404040));
    g.drawRect (getLocalBounds().toFloat().reduced (1.5f), 1.0f);

    // Divider between left (pitch) and right (color) sections
    g.setColour (juce::Colour (0xff383838));
    g.drawLine (static_cast<float> (W / 2 + 10), 40.0f, static_cast<float> (W / 2 + 10),
                static_cast<float> (H - 30), 1.0f);

    // Title
    g.setColour (juce::Colour (0xffc8c8c8));
    g.setFont (juce::Font (16.0f, juce::Font::bold));
    g.drawText ("Varispeed  1.1", 0, 8, W, 20, juce::Justification::centred, false);

    // Section labels
    g.setFont (juce::Font (10.0f, juce::Font::plain));
    g.setColour (juce::Colour (0xff666666));
    g.drawText ("PITCH", 10, 36, W / 2 - 10, 14, juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (0xff6b4800));
    g.drawText ("COLOR", W / 2 + 20, 36, W / 2 - 30, 14, juce::Justification::centredLeft, false);
}

//==============================================================================
void VeriSpeedEditor::resized()
{
    const int W = getWidth();
    const int H = getHeight();

    // ── Tempo button top-left ─────────────────────────────────────────────────
    tempoButton.setBounds (10, 8, 90, 22);

    // ── Author bottom-right ───────────────────────────────────────────────────
    authorLabel.setBounds (W - 185, H - 18, 180, 14);

    // ── LEFT SECTION: Pitch ───────────────────────────────────────────────────
    const int leftW   = W / 2;
    const int vuW     = 120;
    const int vuH     = 90;
    const int vuX     = (leftW - vuW) / 2;
    const int vuY     = 52;

    vuMeter.setBounds (vuX, vuY, vuW, vuH);

    // Step buttons flanking the VU meter
    semitoneDownBtn.setBounds (vuX - 52, vuY + 28, 48, 30);
    semitoneUpBtn.setBounds   (vuX + vuW + 4, vuY + 28, 48, 30);

    // Hidden knob (same bounds as VU meter, for ctrl-drag fine-tuning)
    semitoneKnob.setBounds (vuX, vuY, vuW, vuH);

    // Semitone readout
    semitoneLabel.setBounds ((leftW - 140) / 2, vuY + vuH + 4, 140, 26);

    // Reset
    resetButton.setBounds ((leftW - 110) / 2, vuY + vuH + 34, 110, 22);

    // Exclude row
    const int exY = vuY + vuH + 62;
    excludeLabel.setBounds  (4,   exY, 60, 20);
    excludeEditor.setBounds (66,  exY, 55, 20);

    // ── RIGHT SECTION: Shae Color ─────────────────────────────────────────────
    const int rightX  = leftW + 20;
    const int rightW  = W - rightX - 10;
    const int knobSz  = 100;
    const int knobX   = rightX + (rightW - knobSz) / 2;
    const int knobY   = 52;

    shaeColorKnob.setBounds      (knobX, knobY, knobSz, knobSz);
    shaeColorLabel.setBounds     (rightX, knobY + knobSz + 4, rightW, 16);
    shaeColorValueLabel.setBounds(rightX, knobY + knobSz + 22, rightW, 22);
}
