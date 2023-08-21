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
    CustomVerticalSlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,juce::Slider::NoTextBox)
    {

    }
};

//==============================================================================
/**
*/
class OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::Slider::Listener,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
{
public:
    OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor(OdinsSuperCoolAllPurposeAudioPluginAudioProcessor&);
    ~OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    
    void parameterValueChanged(int parameterIndex, float newValue) override;

    /** Indicates that a parameter change gesture has started.

        E.g. if the user is dragging a slider, this would be called with gestureIsStarting
        being true when they first press the mouse button, and it will be called again with
        gestureIsStarting being false when they release it.

        IMPORTANT NOTE: This will be called synchronously, and many audio processors will
        call it during their audio callback. This means that not only has your handler code
        got to be completely thread-safe, but it's also got to be VERY fast, and avoid
        blocking. If you need to handle this event on your message thread, use this callback
        to trigger an AsyncUpdater or ChangeBroadcaster which you can respond to later on the
        message thread.
    */
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
    void timerCallback() override;

    void sliderValueChanged(juce::Slider* slider) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    OdinsSuperCoolAllPurposeAudioPluginAudioProcessor& audioProcessor;

    juce::Atomic<bool> parametersChanged{ false };
    
    CustomVerticalSlider peakFreqSlider, peakGainSlider, peakQualitySlider, lowCutFreqSlider, highCutFreqSlider, lowCutSlopeSlider, HighCutSlopeSlider;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment peakFreqSliderAttachment, peakGainSliderAttachment, peakQualitySliderAttachment, lowCutFreqSliderAttachment, highCutFreqSliderAttachment, lowCutSlopeSliderAttachment, HighCutSlopeSliderAttachment;

    MonoChain monoChain;

    juce::Slider gainSlider;
    juce::Label GainLabel;

    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor)
};
