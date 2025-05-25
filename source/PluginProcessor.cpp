#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PluginProcessor::PluginProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
                     apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // caching parameter pointers
    grainRateParam = apvts.getRawParameterValue("grainRate");
    grainSizeParam = apvts.getRawParameterValue("grainSize");
    delayTimeParam = apvts.getRawParameterValue("delayTime");
    pitchShiftParam = apvts.getRawParameterValue("pitchShift");
    feedbackParam = apvts.getRawParameterValue("feedback");
    wetDryParam = apvts.getRawParameterValue("wetDry");
}

PluginProcessor::~PluginProcessor()
{
}

//=======================create parameter layout================================

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("grainRate", "Grain Rate", 0.1f, 10.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("grainSize", "Grain Size", 0.1f, 10.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayTime", "Delay Time", 0.1f, 10.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pitchShift", "Pitch Shift", -12.0f, 12.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("feedback", "Feedback", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("wetDry", "Wet/Dry Mix", 0.0f, 1.0f, 0.5f));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need.
    circularBuffer.setSize(getTotalNumInputChannels(), bufferSize);
    circularBuffer.clear();
    writeIndex = 0;
}

void PluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{

    juce::AudioBuffer<float> dryBuffer(buffer);
    buffer.clear();

    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    // initialize parameters as variables
    const float grainSize = *grainSizeParam * sampleRate;
    const float delayTime = *delayTimeParam;
    int delaySamples = static_cast<int>((delayTime / 1000.0f) * sampleRate);
    const float pitchShift = *pitchShiftParam;
    const float spawnRate = *grainRateParam;
    const float feedback = *feedbackParam;
    const float wetDry = *wetDryParam;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        juce::ignoreUnused (channelData);

        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        // write input to the circular buffer
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* in = buffer.getReadPointer(ch);
            auto* cb = circularBuffer.getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                cb[(writeIndex + i) % bufferSize] = in[i];
            }
        }

        // spawn new grains
        for (int i = 0; i < numSamples; ++i)
        {
            grainSpawnCounter += 1.0f / sampleRate;
            if (grainSpawnCounter >= spawnRate)
            {
                grainSpawnCounter = 0.0f;

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    Grain g;
                    g.channel = ch;
                    g.length = grainSize;
                    g.startSample = static_cast<float> ((writeIndex + i + bufferSize - delaySamples) % bufferSize);
                    g.position = 0;
                    g.playbackRate = pitchShift;
                    g.isActive = true;
                    g.envelope.resize((int)g.length);

                    for (int e = 0; e < g.length; ++e)
                    {
                        float norm = e / g.length;
                        g.envelope[e] = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * norm)); // Hann window
                    }
                    activeGrains.push_back(g);
                }
            }
        }

        // clear buffer for output
        buffer.clear();

        // process grains
        for (auto& grain : activeGrains)
        {
            if (!grain.isActive) continue;
            for (int i = 0; i < numSamples; ++i)
            {
                int grainSampleIndex = static_cast<int> (grain.startSample + grain.position) % bufferSize;
                if (grainSampleIndex < 0 || grainSampleIndex >= bufferSize)
                    continue;

                if ((grain.position) < grain.length)
                {
                    float env = grain.envelope[static_cast<int> (grain.position)];
                    float val = circularBuffer.getSample(grain.channel, grainSampleIndex) * env;

                    buffer.addSample(grain.channel, i, val);
                    grain.advance();
                }
                else
                {
                    grain.isActive = false;
                    break;
                }
            }
        }
        // remove dead grains
        activeGrains.erase(
            std::remove_if(activeGrains.begin(), activeGrains.end(),
                [](const Grain& g) { return !g.isActive;}),
                activeGrains.end());

        // wet/dry mix
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            auto* dry = dryBuffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                wet[i] = wetDry * wet[i] + (1.0f - wetDry) * dry[i];
            }
        }

        // feedback
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* cb = circularBuffer.getWritePointer(ch);
            auto* wet = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                int idx = (writeIndex + i) % bufferSize;
                cb[idx] += wet[i] * feedback;
            }
        }

        writeIndex = (writeIndex + numSamples) % bufferSize;
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
