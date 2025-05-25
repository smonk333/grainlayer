#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PluginProcessor& processorRef;
    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton { "Inspect the UI" };

    // set up sliders
    juce::Slider grainSizeSlider,
                 delayTimeSlider,
                 pitchShiftSlider,
                 grainRateSlider,
                 feedbackSlider,
                 wetDrySlider;

    // set up slider labels
    juce::Label grainSizeLabel,
                 delayTimeLabel,
                 pitchShiftLabel,
                 grainRateLabel,
                 feedbackLabel,
                 wetDryLabel;

    // set up slider attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainSizeSliderAttach,
        delayTimeSliderAttach,
        pitchShiftSliderAttach,
        grainRateSliderAttach,
        feedbackSliderAttach,
        wetDrySliderAttach;;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
