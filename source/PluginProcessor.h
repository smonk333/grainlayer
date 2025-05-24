#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#if (MSVC)
#include "ipps.h"
#endif

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================parameter setup=================================

    std::atomic<float>* grainSizeParam;
    std::atomic<float>* delayTimeParam;
    std::atomic<float>* pitchShiftParam;
    std::atomic<float>* grainRateParam;
    std::atomic<float>* feedbackParam;
    std::atomic<float>* wetDryParam;
    juce::AudioProcessorValueTreeState apvts;

    //============================grain struct==================================

    struct Grain
    {
        float startSample;
        float length;
        float playbackRate;
        float position;
        std::vector<float> envelope;
        bool isActive = true;

        float getSample (const juce::AudioBuffer<float>& source);


        [[nodiscard]] float getEnvelopeValue() const
        {
            if (position >= length) return 0.0f;
            float norm = position / length;
            return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * norm));
        }

        void advance() {position += playbackRate;}

    };

    //========================circular buffer setup=============================

    const int bufferSize = 48000 * 2; // 2 seconds of buffer @ 48kHz
    juce::AudioBuffer<float> circularBuffer;
    int writeIndex = 0;

    std::vector<Grain> activeGrains;

    double sampleRate = 44100.0;
    float grainSpawnCounter = 0.0f;
    float spawnRate = 0.02f;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
