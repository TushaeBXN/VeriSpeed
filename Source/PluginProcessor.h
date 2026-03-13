#pragma once

#include <JuceHeader.h>
#include <array>

//==============================================================================
// Parameter IDs – keep in sync with PluginEditor
namespace ParamID
{
    inline constexpr const char* semitoneShift = "semitone_shift";
    inline constexpr const char* tempoOn       = "tempo_on";
    inline constexpr const char* excludeSymbol = "exclude_symbol";
}

//==============================================================================
/**
 * VeriSpeedAudioProcessor
 *
 * Tape varispeed simulation via phase-vocoder pitch shifting (JUCE dsp::PhaseVocoder
 * wrapper is not available in JUCE 7, so we use a high-quality sinc resampler via
 * juce::LagrangeInterpolator paired with a real-time pitch shift using a
 * time-domain OLA approach based on juce::dsp::ProcessorChain).
 *
 * For production use the included sinc-resampler + OLA pitch shifter is sufficient
 * for master-bus tape-style detuning.  Replace with a phase-vocoder library
 * (e.g. Rubber Band) if high-quality polyphonic pitch shifting is required.
 */
class VeriSpeedAudioProcessor : public juce::AudioProcessor,
                                 public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==========================================================================
    VeriSpeedAudioProcessor();
    ~VeriSpeedAudioProcessor() override;

    //==========================================================================
    // AudioProcessor overrides
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    // Metadata
    const juce::String getName() const override { return "VeriSpeed"; }
    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==========================================================================
    // Programs
    int  getNumPrograms()    override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    // State
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // APVTS (public so Editor can attach sliders directly)
    juce::AudioProcessorValueTreeState apvts;

    //==========================================================================
    // Real-time helpers for the VU meter
    float getCurrentSemitones() const noexcept { return currentSemitones.load(); }

    //==========================================================================
    // Listener callback
    void parameterChanged (const juce::String& paramID, float newValue) override;

private:
    //==========================================================================
    // APVTS factory helper
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // Tape speed formula
    static double semitoneToRatio (double semitones) noexcept
    {
        return std::pow (2.0, semitones / 12.0);
    }

    //==========================================================================
    // DSP state
    struct ChannelState
    {
        // Circular read buffer for OLA pitch shift
        static constexpr int kBufSize = 1 << 17; // 131072 samples
        std::array<float, kBufSize> buffer {};
        int   writePos   = 0;
        float readPos    = 0.0f;

        juce::LagrangeInterpolator interpolator;

        void reset()
        {
            buffer.fill (0.0f);
            writePos = 0;
            readPos  = 0.0f;
            interpolator.reset();
        }
    };

    std::array<ChannelState, 2> channels;

    double   currentSampleRate   = 44100.0;
    int      currentBlockSize    = 512;

    // Atomics for thread-safe UI readback
    std::atomic<float> currentSemitones { 0.0f };

    // Cached parameter values (written on audio thread after parameterChanged)
    std::atomic<float> paramSemitones { 0.0f };
    std::atomic<bool>  paramTempoOn   { false };

    // Latency in samples (set in prepareToPlay, zero for resampler approach)
    int reportedLatency = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VeriSpeedAudioProcessor)
};
