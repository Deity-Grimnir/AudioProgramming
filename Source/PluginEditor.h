/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomVerticalSlider : juce::Slider
{
    CustomVerticalSlider() : juce::Slider(juce::Slider::SliderStyle::Rotary,juce::Slider::NoTextBox)
    {

    }
};
struct ResponseCurveComponent : juce::Component,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
{
    ResponseCurveComponent(OdinsSuperCoolAllPurposeAudioPluginAudioProcessor&);
    ~ResponseCurveComponent();
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
    void timerCallback() override;

    void paint(juce::Graphics& g) override;
private:
    OdinsSuperCoolAllPurposeAudioPluginAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged{ false };

    MonoChain monoChain;
};

//==============================================================================
/**
*/
class OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor,public juce::Slider::Listener
{
public:
    OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor(OdinsSuperCoolAllPurposeAudioPluginAudioProcessor&);
    ~OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override ;
    
  

    void sliderValueChanged(juce::Slider* slider) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    OdinsSuperCoolAllPurposeAudioPluginAudioProcessor& audioProcessor;


    
    CustomVerticalSlider peakFreqSlider, peakGainSlider, peakQualitySlider, lowCutFreqSlider, highCutFreqSlider, lowCutSlopeSlider, HighCutSlopeSlider;

    ResponseCurveComponent responseCurveComponent;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment peakFreqSliderAttachment, peakGainSliderAttachment, peakQualitySliderAttachment, lowCutFreqSliderAttachment, highCutFreqSliderAttachment, lowCutSlopeSliderAttachment, HighCutSlopeSliderAttachment;



    juce::Slider gainSlider;
    juce::Label GainLabel;

    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor)
};
