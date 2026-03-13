#pragma once

#include <JuceHeader.h>
#include <array>

//==============================================================================
namespace ParamID
{
    inline constexpr const char* semitoneShift = "semitone_shift";
    inline constexpr const char* tempoOn       = "tempo_on";
    inline constexpr const char* shaeColor     = "shae_color";
    inline constexpr const char* excludeSymbol = "exclude_symbol";
}

//==============================================================================
class VeriSpeedAudioProcessor : public juce::AudioProcessor,
                                 public juce::AudioProcessorValueTreeState::Listener
{
public:
    VeriSpeedAudioProcessor();
    ~VeriSpeedAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VeriSpeed"; }
    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.5; } // reverb tail

    int  getNumPrograms()    override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    float getCurrentSemitones() const noexcept { return currentSemitones.load(); }
    void parameterChanged (const juce::String& paramID, float newValue) override;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    static double semitoneToRatio (double semitones) noexcept
    {
        return std::pow (2.0, semitones / 12.0);
    }

    //==========================================================================
    // Pitch shifter state
    struct ChannelState
    {
        static constexpr int kBufSize = 1 << 17;
        std::array<float, kBufSize> buffer {};
        int   writePos = 0;
        float readPos  = 0.0f;
        juce::LagrangeInterpolator interpolator;
        void reset() { buffer.fill (0.0f); writePos = 0; readPos = 0.0f; interpolator.reset(); }
    };
    std::array<ChannelState, 2> channels;

    //==========================================================================
    // Shae Color DSP chain (using JUCE DSP module)
    // Order: Compressor → Saturation (WaveShaper) → Chorus → Reverb → Width
    using StereoContext = juce::dsp::ProcessContextReplacing<float>;

    // Multiband-style soft compression (single band, gain-reduction style)
    juce::dsp::Compressor<float> compressor;

    // Soft saturation waveshaper
    juce::dsp::WaveShaper<float> saturator;

    // Vintage chorus (delay-line LFO)
    juce::dsp::Chorus<float> chorus;

    // Reverb (juce::Reverb handles stereo natively via processStereo)
    juce::Reverb reverb;

    // State for LFO pan (PanOmatic-style stereo movement)
    float lfoPhase = 0.0f;

    // Prepare flag
    bool dspPrepared = false;

    //==========================================================================
    // Smoothed color value (avoids zipper noise on knob turn)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedColor;

    //==========================================================================
    // Cached params
    std::atomic<float> currentSemitones { 0.0f };
    std::atomic<float> paramSemitones   { 0.0f };
    std::atomic<bool>  paramTempoOn     { false };
    std::atomic<float> paramShaeColor   { 0.0f };

    double currentSampleRate = 44100.0;
    int    currentBlockSize  = 512;

    //==========================================================================
    // Helpers
    void applyShaeColor (juce::AudioBuffer<float>& buffer, float colorAmt);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VeriSpeedAudioProcessor)
};
