#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Parameter layout
juce::AudioProcessorValueTreeState::ParameterLayout
VeriSpeedAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Semitone shift  –24 … +24,  default 0
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::semitoneShift, 1 },
        "Semitone Shift",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes{}
            .withLabel ("st")
    ));

    // Tempo on/off
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::tempoOn, 1 },
        "Tempo On",
        false
    ));

    // Exclude symbol (string param via choice – stored as free text via custom param)
    // JUCE APVTS doesn't have a native String param, so we expose it as a
    // StringValueTreeState entry and handle serialisation manually.
    // (UI stores exclude text in ValueTree directly.)

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

    // Initialise cached values from APVTS defaults
    paramSemitones.store (*apvts.getRawParameterValue (ParamID::semitoneShift));
    paramTempoOn.store   (static_cast<bool> (
        *apvts.getRawParameterValue (ParamID::tempoOn) > 0.5f));
}

VeriSpeedAudioProcessor::~VeriSpeedAudioProcessor()
{
    apvts.removeParameterListener (ParamID::semitoneShift, this);
    apvts.removeParameterListener (ParamID::tempoOn,       this);
}

//==============================================================================
void VeriSpeedAudioProcessor::parameterChanged (const juce::String& paramID, float newValue)
{
    if (paramID == ParamID::semitoneShift)
        paramSemitones.store (newValue);
    else if (paramID == ParamID::tempoOn)
        paramTempoOn.store (newValue > 0.5f);
}

//==============================================================================
void VeriSpeedAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize  = samplesPerBlock;

    for (auto& ch : channels)
        ch.reset();

    // Resampler approach has zero inherent latency
    reportedLatency = 0;
    setLatencySamples (reportedLatency);
}

void VeriSpeedAudioProcessor::releaseResources()
{
    for (auto& ch : channels)
        ch.reset();
}

//==============================================================================
bool VeriSpeedAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Stereo in / stereo out only
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

//==============================================================================
/**
 * Core DSP:
 * 1. Read semitone parameter → compute speed ratio via tape formula.
 * 2. For each channel, write incoming samples into a circular buffer and read
 *    back at a rate scaled by speed_ratio using Lagrange interpolation.
 *    - ratio > 1: read faster → fewer output samples per input → pitch up
 *    - ratio < 1: read slower → pitch down
 *    We always produce exactly samplesPerBlock output samples by adjusting the
 *    read increment (speedRatio) and zero-padding or stretching accordingly.
 * 3. If Tempo_on, signal the host via PlayHead (informational only – VST3 hosts
 *    control their own transport; we annotate the playhead info for hosts that
 *    support tempo scaling via plugin, e.g. FL Studio tempo follower).
 */
void VeriSpeedAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);

    // ── Read parameters ──────────────────────────────────────────────────────
    const float semitones  = paramSemitones.load();
    const double speedRatio = std::pow (2.0, static_cast<double> (semitones) / 12.0);

    // Export live semitone value for VU meter
    currentSemitones.store (semitones);

    // ── Tempo reporting ───────────────────────────────────────────────────────
    if (paramTempoOn.load())
    {
        if (auto* playHead = getPlayHead())
        {
            // We read the current BPM; in FL Studio the plugin cannot write
            // the host BPM directly via VST3, but we expose the scaled BPM
            // in the ValueTree so the UI can display it.
            if (auto pos = playHead->getPosition())
            {
                if (pos->getBpm().hasValue())
                {
                    const double scaledBpm = *pos->getBpm() * speedRatio;
                    // Store for UI display (non-audio thread reads this)
                    apvts.state.setProperty ("scaledBpm",
                                             static_cast<float> (scaledBpm),
                                             nullptr);
                }
            }
        }
    }

    // ── Bypass (no shift) fast path ──────────────────────────────────────────
    if (std::abs (semitones) < 0.005f)
    {
        // pass-through — no processing needed
        return;
    }

    // ── Per-channel resampling pitch shift ────────────────────────────────────
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto& state  = channels[static_cast<size_t> (ch)];
        float* data  = buffer.getWritePointer (ch);

        constexpr int kBufMask = ChannelState::kBufSize - 1;

        // Write all input samples into circular buffer
        for (int i = 0; i < numSamples; ++i)
        {
            state.buffer[state.writePos & kBufMask] = data[i];
            ++state.writePos;
        }

        // Read back at scaled rate using Lagrange interpolation.
        // We start reading from (writePos - numSamples) and step by speedRatio.
        const float startRead = static_cast<float> (state.writePos - numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            const float fPos = startRead + static_cast<float> (i) * static_cast<float> (speedRatio);

            // Integer and fractional parts
            const int   iPos = static_cast<int> (fPos);
            const float frac = fPos - static_cast<float> (iPos);

            // 4-point Lagrange interpolation
            const float s0 = state.buffer[(iPos - 1) & kBufMask];
            const float s1 = state.buffer[(iPos    ) & kBufMask];
            const float s2 = state.buffer[(iPos + 1) & kBufMask];
            const float s3 = state.buffer[(iPos + 2) & kBufMask];

            // Catmull-Rom / Lagrange cubic
            const float a = s3 - s2 - s0 + s1;
            const float b = s0 - s1 - a;
            const float c = s2 - s0;
            const float d = s1;

            data[i] = a * frac * frac * frac
                    + b * frac * frac
                    + c * frac
                    + d;
        }
    }
}

void VeriSpeedAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer,
                                                     juce::MidiBuffer& midiMessages)
{
    // Reset interpolators so no glitch on re-enable
    for (auto& ch : channels)
        ch.reset();

    AudioProcessor::processBlockBypassed (buffer, midiMessages);
}

//==============================================================================
juce::AudioProcessorEditor* VeriSpeedAudioProcessor::createEditor()
{
    return new VeriSpeedEditor (*this);
}

//==============================================================================
void VeriSpeedAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Persist APVTS + exclude symbol
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

//==============================================================================
// Plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VeriSpeedAudioProcessor();
}
