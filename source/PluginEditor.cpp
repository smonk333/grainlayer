#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    auto& params = processorRef.apvts;

    auto setupSlider = [this](juce::Slider& s) {
        s.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible(s);
    };

    auto setupLabel = [this](juce::Label& l, const juce::String& text) {
      l.setText(text, juce::dontSendNotification);
      l.setJustificationType(juce::Justification::centred);
      addAndMakeVisible(l);
    };

    setupSlider(grainSizeSlider);
    setupSlider(delayTimeSlider);
    setupSlider(pitchShiftSlider);
    setupSlider(grainRateSlider);
    setupSlider(feedbackSlider);
    setupSlider(wetDrySlider);

    setupLabel(grainSizeLabel, "Grain Size");
    setupLabel(delayTimeLabel, "Delay Time");
    setupLabel(pitchShiftLabel, "Pitch Shift");
    setupLabel(grainRateLabel, "Grain Rate");
    setupLabel(feedbackLabel, "Feedback");
    setupLabel(wetDryLabel, "Wet/Dry Mix");

    grainSizeSliderAttach = std::make_unique<SliderAttachment>(params, "grainSize", grainSizeSlider);
    delayTimeSliderAttach = std::make_unique<SliderAttachment>(params, "delayTime", delayTimeSlider);
    pitchShiftSliderAttach = std::make_unique<SliderAttachment>(params, "pitchShift", pitchShiftSlider);
    grainRateSliderAttach = std::make_unique<SliderAttachment>(params, "grainRate", grainRateSlider);
    feedbackSliderAttach = std::make_unique<SliderAttachment>(params, "feedback", feedbackSlider);
    wetDrySliderAttach = std::make_unique<SliderAttachment>(params, "wetDry", wetDrySlider);

    addAndMakeVisible (inspectButton);

    // this chunk of code instantiates and opens the melatonin inspector
    inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible (true);
    };

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    setResizable(true, true);
}

PluginEditor::~PluginEditor()
{
}

void PluginEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

}

void PluginEditor::resized()
{
    // layout the positions of your child components here
    auto area = getLocalBounds().reduced(20);
    auto row = area.removeFromTop(220);

    auto sliderWidth = row.getWidth() / 6;

    grainSizeSlider.setBounds(row.removeFromLeft(sliderWidth));
    delayTimeSlider.setBounds(row.removeFromLeft(sliderWidth));
    pitchShiftSlider.setBounds(row.removeFromLeft(sliderWidth));
    grainRateSlider.setBounds(row.removeFromLeft(sliderWidth));
    feedbackSlider.setBounds(row.removeFromLeft(sliderWidth));
    wetDrySlider.setBounds(row.removeFromLeft(sliderWidth));

    auto labelRow = getLocalBounds().reduced(20).removeFromBottom(180 + 0).removeFromBottom(30);
    auto labelWidth = labelRow.getWidth() / 6;

    grainSizeLabel.setBounds(labelRow.removeFromLeft(labelWidth));
    delayTimeLabel.setBounds(labelRow.removeFromLeft(labelWidth));
    pitchShiftLabel.setBounds(labelRow.removeFromLeft(labelWidth));
    grainRateLabel.setBounds(labelRow.removeFromLeft(labelWidth));
    feedbackLabel.setBounds(labelRow.removeFromLeft(labelWidth));
    wetDryLabel.setBounds(labelRow.removeFromLeft(labelWidth));

    area.removeFromBottom(50);
    inspectButton.setBounds (getLocalBounds().withSizeKeepingCentre(100, 50));
}
