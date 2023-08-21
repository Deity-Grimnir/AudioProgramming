/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor::OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor (OdinsSuperCoolAllPurposeAudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for (auto* comp : getComps())
    { 
        addAndMakeVisible(comp);
    }

    setSize (600, 400);

    gainSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 100, 25);
    gainSlider.setRange(-48.0, 0.0);
    gainSlider.setValue(-1.0);
    gainSlider.setSize(250, 20.0);
    gainSlider.addListener(this);
    addAndMakeVisible(gainSlider);

    GainLabel.setText("Volume", juce::dontSendNotification);
    addAndMakeVisible(GainLabel);
    GainLabel.attachToComponent(&gainSlider, true);
}

OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor::~OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    //g.setColour(juce::Colours::orange);
    //g.setFont(20.0f);


    //g.drawFittedText("Volume", getLocalBounds(),gainSlider.getTextBoxHeight(), 1);
   
}

void OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto sliderLeft = 120;
    gainSlider.setBounds(sliderLeft, 20, getWidth() - sliderLeft - 10, 50);

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

    auto LowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto HighCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.5);

    lowCutFreqSlider.setBounds(LowCutArea);
    highCutFreqSlider.setBounds(HighCutArea);

    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);

}

std::vector<juce::Component*> OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider
    };

}

void OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &gainSlider)
    {
        audioProcessor.rawVolume = pow(10, gainSlider.getValue() / 20);
    }
}