#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>

class StereoToolAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int scopeSize = 4096; // potencia de 2

    StereoToolAudioProcessor();
    ~StereoToolAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Stereo Tool Pro"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;

    // ---- Datos para la interfaz ----
    // indices: 0 = L in, 1 = R in, 2 = L out, 3 = R out
    std::atomic<float> level[4] {};
    std::atomic<float> peakMax[4] {};
    std::atomic<float> correlation { 0.0f };

    float scopeL[scopeSize] {};
    float scopeR[scopeSize] {};
    std::atomic<int> scopeWritePos { 0 };

private:
    static float dbToGain (float db) noexcept
    {
        return db <= -95.99f ? 0.0f : juce::Decibels::decibelsToGain (db);
    }

    std::atomic<float>* pGainL = nullptr;
    std::atomic<float>* pGainR = nullptr;
    std::atomic<float>* pPanL = nullptr;
    std::atomic<float>* pPanR = nullptr;
    std::atomic<float>* pPhaseL = nullptr;
    std::atomic<float>* pPhaseR = nullptr;
    std::atomic<float>* pWidth = nullptr;
    std::atomic<float>* pPan = nullptr;

    juce::SmoothedValue<float> sGainL, sGainR, sPanL, sPanR, sPolL, sPolR, sWidth, sPan;

    float env[4] { 0, 0, 0, 0 };
    float attackCoeff = 0.1f, releaseCoeff = 0.9999f;
    double sLL = 0.0, sRR = 0.0, sLR = 0.0, corrCoeff = 0.001;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoToolAudioProcessor)
};
