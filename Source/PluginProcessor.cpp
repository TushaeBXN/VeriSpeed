#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
VeriSpeedAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::semitoneShift, 1 },
        "Semitone Shift",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes{}.withLabel ("st")
    ));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::tempoOn, 1 },
        "Tempo On",
        false
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::shaeColor, 1 },
        "Shae Color",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes{}.withLabel ("")
    ));

    return layout;
}

//==============================================================================
VeriSpeedAudioProcessor::VeriSpeedAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VeriSpeedState", createParameterLayout())
{
    apvts.addParameterListener (ParamID::semitoneShift, this);
    apvts.addParameterListener (ParamID::tempoOn,       this);
    apvts.addParameterListener (ParamID::shaeColor,     this);

    paramSemitones.store (*apvts.getRawParameterValue (ParamID::semitoneShift));
    paramTempoOn.store   (*apvts.getRawParameterValue (ParamID::tempoOn) > 0.5f);
    paramShaeColor.store (*apvts.getRawParameterValue (ParamID::shaeColor));
}

VeriSpeedAudioProcessor::~VeriSpeedAudioProcessor()
{
    apvts.removeParameterListener (ParamID::semitoneShift, this);
    apvts.removeParameterListener (ParamID::tempoOn,       this);
    apvts.removeParameterListener (ParamID::shaeColor,     this);
}

//==============================================================================
void VeriSpeedAudioProcessor::parameterChanged (const juce::String& paramID, float newValue)
{
    if      (paramID == ParamID::semitoneShift) paramSemitones.store (newValue);
    else if (paramID == ParamID::tempoOn)       paramTempoOn.store (newValue > 0.5f);
    else if (paramID == ParamID::shaeColor)     paramShaeColor.store (newValue);
}

//==============================================================================
void VeriSpeedAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize  = samplesPerBlock;

    for (auto& ch : channels)
        ch.reset();

    setLatencySamples (0);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = 2;

    // ── Compressor (Soundgoodizer / Maximus-style) ────────────────────────────
    compressor.prepare (spec);
    compressor.setThreshold (-18.0f);
    compressor.setRatio     (4.0f);
    compressor.setAttack    (10.0f);
    compressor.setRelease   (150.0f);

    // ── Saturator (soft tape saturation) ─────────────────────────────────────
    saturator.prepare (spec);
    saturator.functionToUse = [] (float x) -> float {
        // Soft clip: tanh-based, gentle harmonic distortion
        return std::tanh (x);
    };

    // ── Chorus (Vintage Chorus + Hyper Chorus blend) ──────────────────────────
    chorus.prepare (spec);
    chorus.setRate      (0.6f);   // slow LFO like vintage chorus
    chorus.setDepth     (0.25f);
    chorus.setCentreDelay (7.0f); // ms, vintage bucket-brigade feel
    chorus.setFeedback  (0.15f);
    chorus.setMix       (0.5f);

    // ── Reverb (LuxeVerb-inspired) ────────────────────────────────────────────
    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize   = 0.55f;
    reverbParams.damping    = 0.45f;
    reverbParams.wetLevel   = 0.0f;
    reverbParams.dryLevel   = 1.0f;
    reverbParams.width      = 0.8f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters (reverbParams);
    reverb.reset();

    // ── Smoothed color ────────────────────────────────────────────────────────
    smoothedColor.reset (sampleRate, 0.05); // 50ms smoothing
    smoothedColor.setCurrentAndTargetValue (paramShaeColor.load());

    lfoPhase    = 0.0f;
    dspPrepared = true;
}

void VeriSpeedAudioProcessor::releaseResources()
{
    for (auto& ch : channels)
        ch.reset();
    dspPrepared = false;
}

//==============================================================================
bool VeriSpeedAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    if (layouts.getMainInputChannelSet()  != juce::AudioChannelSet::stereo()) return false;
    return true;
}

//==============================================================================
/**
 * Shae Color processing — 0..10 knob maps progressively:
 *
 *  0.0 – 2.0  : Compression + gain makeup (Soundgoodizer / Maximus glue)
 *  2.0 – 4.0  : Soft tape saturation fades in (SAT knob)
 *  4.0 – 6.0  : Stereo LFO pan width (PanOmatic-style movement)
 *  6.0 – 8.0  : Vintage chorus + Hyper chorus blended in
 *  8.0 – 10.0 : LuxeVerb reverb wet signal rises
 */
void VeriSpeedAudioProcessor::applyShaeColor (juce::AudioBuffer<float>& buffer, float color)
{
    if (!dspPrepared) return;

    const int numSamples  = buffer.getNumSamples();
    const float norm      = color / 10.0f; // 0..1

    // ── 1. Compression (kicks in 0→1) ─────────────────────────────────────────
    if (color > 0.01f)
    {
        const float compAmt = juce::jmin (norm * 5.0f, 1.0f); // full by color=2
        compressor.setThreshold (juce::jmap (compAmt, 0.0f, 1.0f, -6.0f, -24.0f));
        compressor.setRatio     (juce::jmap (compAmt, 0.0f, 1.0f, 1.5f,  6.0f));

        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        compressor.process (ctx);

        // Gain makeup to compensate compression
        const float makeupGain = juce::Decibels::decibelsToGain (compAmt * 4.0f);
        buffer.applyGain (makeupGain);
    }

    // ── 2. Tape saturation (kicks in color > 2) ───────────────────────────────
    if (color > 2.0f)
    {
        const float satAmt = juce::jmin ((color - 2.0f) / 2.0f, 1.0f); // full by color=4

        // Drive into waveshaper then blend wet/dry
        juce::AudioBuffer<float> satBuffer (buffer.getNumChannels(), numSamples);
        satBuffer.copyFrom (0, 0, buffer, 0, 0, numSamples);
        satBuffer.copyFrom (1, 0, buffer, 1, 0, numSamples);

        // Pre-gain into saturator
        const float driveGain = 1.0f + satAmt * 2.5f;
        satBuffer.applyGain (driveGain);

        juce::dsp::AudioBlock<float> satBlock (satBuffer);
        juce::dsp::ProcessContextReplacing<float> satCtx (satBlock);
        saturator.process (satCtx);

        // Compensate output level
        satBuffer.applyGain (1.0f / driveGain);

        // Blend
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            buffer.addFromWithRamp (ch, 0, satBuffer.getReadPointer (ch),
                                    numSamples, satAmt * 0.5f, satAmt * 0.5f);
        }
    }

    // ── 3. Stereo LFO width / PanOmatic movement (color > 4) ─────────────────
    if (color > 4.0f && buffer.getNumChannels() >= 2)
    {
        const float widthAmt = juce::jmin ((color - 4.0f) / 2.0f, 1.0f);
        const float lfoRate  = 0.4f; // Hz
        const float lfoDepth = widthAmt * 0.18f;

        float* L = buffer.getWritePointer (0);
        float* R = buffer.getWritePointer (1);

        for (int i = 0; i < numSamples; ++i)
        {
            lfoPhase += lfoRate / static_cast<float> (currentSampleRate);
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

            const float lfo = std::sin (lfoPhase * juce::MathConstants<float>::twoPi);
            const float panL = 1.0f + lfo * lfoDepth;
            const float panR = 1.0f - lfo * lfoDepth;

            L[i] *= panL;
            R[i] *= panR;
        }
    }

    // ── 4. Chorus (color > 6) ─────────────────────────────────────────────────
    if (color > 6.0f)
    {
        const float chorusAmt = juce::jmin ((color - 6.0f) / 2.0f, 1.0f);
        chorus.setMix (chorusAmt * 0.45f);
        chorus.setDepth (0.15f + chorusAmt * 0.25f);

        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        chorus.process (ctx);
    }

    // ── 5. Reverb (color > 8) ─────────────────────────────────────────────────
    if (color > 8.0f)
    {
        const float reverbAmt = juce::jmin ((color - 8.0f) / 2.0f, 1.0f);

        juce::Reverb::Parameters rp;
        rp.roomSize   = 0.55f + reverbAmt * 0.25f;
        rp.damping    = 0.45f;
        rp.wetLevel   = reverbAmt * 0.30f;
        rp.dryLevel   = 1.0f;
        rp.width      = 0.8f + reverbAmt * 0.2f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);

        // juce::Reverb processes stereo natively
        if (buffer.getNumChannels() >= 2)
            reverb.processStereo (buffer.getWritePointer (0),
                                  buffer.getWritePointer (1),
                                  buffer.getNumSamples());
        else
            reverb.processMono (buffer.getWritePointer (0),
                                buffer.getNumSamples());
    }

    // ── Final output safety limiter ───────────────────────────────────────────
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* data = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] = juce::jlimit (-1.0f, 1.0f, data[i]);
    }
}

//==============================================================================
void VeriSpeedAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);

    const float semitones   = paramSemitones.load();
    const double speedRatio = std::pow (2.0, static_cast<double> (semitones) / 12.0);
    currentSemitones.store (semitones);

    // ── Tempo reporting ───────────────────────────────────────────────────────
    if (paramTempoOn.load())
    {
        if (auto* playHead = getPlayHead())
        {
            if (auto pos = playHead->getPosition())
            {
                if (pos->getBpm().hasValue())
                {
                    const double scaledBpm = *pos->getBpm() * speedRatio;
                    apvts.state.setProperty ("scaledBpm", static_cast<float> (scaledBpm), nullptr);
                }
            }
        }
    }

    // ── Pitch shift ───────────────────────────────────────────────────────────
    if (std::abs (semitones) >= 0.005f)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& state = channels[static_cast<size_t> (ch)];
            float* data = buffer.getWritePointer (ch);
            constexpr int kBufMask = ChannelState::kBufSize - 1;

            for (int i = 0; i < numSamples; ++i)
            {
                state.buffer[state.writePos & kBufMask] = data[i];
                ++state.writePos;
            }

            const float startRead = static_cast<float> (state.writePos - numSamples);
            for (int i = 0; i < numSamples; ++i)
            {
                const float fPos = startRead + static_cast<float> (i) * static_cast<float> (speedRatio);
                const int   iPos = static_cast<int> (fPos);
                const float frac = fPos - static_cast<float> (iPos);

                const float s0 = state.buffer[(iPos - 1) & kBufMask];
                const float s1 = state.buffer[(iPos    ) & kBufMask];
                const float s2 = state.buffer[(iPos + 1) & kBufMask];
                const float s3 = state.buffer[(iPos + 2) & kBufMask];

                const float a = s3 - s2 - s0 + s1;
                const float b = s0 - s1 - a;
                const float c = s2 - s0;
                const float d = s1;

                data[i] = a * frac * frac * frac + b * frac * frac + c * frac + d;
            }
        }
    }

    // ── Shae Color ────────────────────────────────────────────────────────────
    smoothedColor.setTargetValue (paramShaeColor.load());
    const float colorAmt = smoothedColor.getNextValue();

    if (colorAmt > 0.01f)
        applyShaeColor (buffer, colorAmt);
}

void VeriSpeedAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer,
                                                     juce::MidiBuffer& midiMessages)
{
    for (auto& ch : channels) ch.reset();
    AudioProcessor::processBlockBypassed (buffer, midiMessages);
}

//==============================================================================
juce::AudioProcessorEditor* VeriSpeedAudioProcessor::createEditor()
{
    return new VeriSpeedEditor (*this);
}

void VeriSpeedAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VeriSpeedAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VeriSpeedAudioProcessor();
}
