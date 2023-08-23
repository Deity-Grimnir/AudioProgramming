/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
void LookAndFeel::drawRotarySlider(juce::Graphics & g,
    int x, int y, int width, int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    using namespace juce;
    auto enabled = slider.isEnabled();
    auto bounds = Rectangle<float>(x, y, width, height);
    g.setColour(juce::Colours::deepskyblue);
    g.fillEllipse(bounds);

    g.setColour(Colour(0u, 0u, 0u));
    g.drawEllipse(bounds,1.f);

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;
        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);

        p.addRoundedRectangle(r, 2.f);

        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());
        g.setColour((juce::Colour(36u, 122u, 163u)));
        g.fillRect(r);

        g.setColour(Colours::white);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);


    }





}

void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    using namespace juce;

    auto startAng = degreesToRadians(180.f + 45.0f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto Range = getRange();

    auto sliderBounds = getSliderBounds();
    //g.setColour(Colours::red);
    //g.drawRect(getLocalBounds());
    //g.setColour(Colours::yellow);
    //g.drawRect(sliderBounds);


    getLookAndFeel().drawRotarySlider(g, sliderBounds.getX(), sliderBounds.getY(), sliderBounds.getWidth(), sliderBounds.getHeight(), jmap(getValue(), Range.getStart(), Range.getEnd(), 0.0, 1.0),
        startAng, endAng, *this);


}
juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();

    auto Size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    Size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(Size, Size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);
     
    return r;
}


juce::String RotarySliderWithLabels::getDisplayString() const
{
    return juce::String(getValue());
}
ResponseCurveComponent::ResponseCurveComponent(OdinsSuperCoolAllPurposeAudioPluginAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }

    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}
void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}
void ResponseCurveComponent::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        auto chainSettings = getChainSettings(audioProcessor.apvts);
        auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<Chainpositions::Peak>().coefficients, peakCoefficients);
        auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
        auto highCutCoefficients = makeHighcutFilter(chainSettings, audioProcessor.getSampleRate());

        updateCutFilter(monoChain.get<Chainpositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
        updateCutFilter(monoChain.get<Chainpositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
        repaint();

    }
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colour(36u, 122u, 163u));   

    auto responseArea = getLocalBounds();
    auto w = responseArea.getWidth();

    auto& lowcut = monoChain.get<Chainpositions::LowCut>();
    auto& peak = monoChain.get<Chainpositions::Peak>();
    auto& highcut = monoChain.get<Chainpositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> mags;

    mags.resize(w);

    for (int i = 0; i < w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

        if (!monoChain.isBypassed<Chainpositions::Peak>())
        {
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lowcut.isBypassed<0>())
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<1>())
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<2>())
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<3>())
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);


        if (!highcut.isBypassed<0>())
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!highcut.isBypassed<1>())
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!highcut.isBypassed<2>())
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!highcut.isBypassed<3>())
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        mags[i] = Decibels::gainToDecibels(mag);
    }

    Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    for (size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }

    g.setColour(Colours::black);



    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 2.f);

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
    g.setColour(Colours::black);
    g.drawRect(0, -1, 667, 482, 2);





}
//==============================================================================
OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor::OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor (OdinsSuperCoolAllPurposeAudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "HZ"),
    peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
    peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
    lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
    highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
    lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
    HighCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),
    driveKnob(*audioProcessor.apvts.getParameter("Drive"), ""),
    rangeKnob(*audioProcessor.apvts.getParameter("Range"), ""),
    blendKnob(*audioProcessor.apvts.getParameter("Blend"), ""),
    volumeKnob(*audioProcessor.apvts.getParameter("Volume"), ""),
    responseCurveComponent(audioProcessor),
    peakFreqSliderAttachment(audioProcessor.apvts,"Peak Freq", peakFreqSlider),
    peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
    peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
    lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
    highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
    lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
    HighCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", HighCutSlopeSlider),
    driveAttachment(audioProcessor.apvts,("drive"), driveKnob),
    rangeAttachment(audioProcessor.apvts,("Range"), rangeKnob),
    blendAttachment(audioProcessor.apvts,("Blend"), driveKnob),
    volumeAttachment(audioProcessor.apvts,("Volume"), volumeKnob)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }


    //addAndMakeVisible(driveKnob = new juce::Slider("Drive"));
    //driveKnob->setSliderStyle(juce::Slider::Rotary);
    //driveKnob->setTextBoxStyle(juce::Slider::NoTextBox, false, 100, 100);

    //addAndMakeVisible(rangeKnob = new juce::Slider("Range"));
    //rangeKnob->setSliderStyle(juce::Slider::Rotary);
    //rangeKnob->setTextBoxStyle(juce::Slider::NoTextBox, false, 100, 100);

    //addAndMakeVisible(blendKnob = new juce::Slider("Blend"));
    //blendKnob->setSliderStyle(juce::Slider::Rotary);
    //blendKnob->setTextBoxStyle(juce::Slider::NoTextBox, false, 100, 100);

    //addAndMakeVisible(volumeKnob = new juce::Slider("Volume"));
    //volumeKnob->setSliderStyle(juce::Slider::Rotary);
    //volumeKnob->setTextBoxStyle(juce::Slider::NoTextBox, false, 100, 100);

    //driveAttachment = new juce::AudioProcessorValueTreeState::SliderAttachment(p.getState(), "drive", *driveKnob);
    //rangeAttachment = new juce::AudioProcessorValueTreeState::SliderAttachment(p.getState(), "range", *rangeKnob);
    //blendAttachment = new juce::AudioProcessorValueTreeState::SliderAttachment(p.getState(), "blend", *blendKnob);
    //volumeAttachment = new juce::AudioProcessorValueTreeState::SliderAttachment(p.getState(), "volume", *volumeKnob);
    setSize (800, 600);

    gainSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 0, 0);
    gainSlider.setRange(-48.0, 0.0);
    gainSlider.setValue(-1.0);
    gainSlider.setSize(500, 100.f);
    gainSlider.addListener(this);
    addAndMakeVisible(gainSlider);

    GainLabel.setText("Volume", juce::dontSendNotification);
    addAndMakeVisible(GainLabel);
    GainLabel.setColour(0,juce::Colours::orange);
    GainLabel.attachToComponent(&gainSlider, true);
}

OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor::~OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor()
{


}

//==============================================================================
void OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{

    g.fillAll(juce::Colours::lightskyblue);


    juce::Colour textColor = juce::Colours::whitesmoke; 
    GainLabel.setColour(juce::Label::textColourId, textColor);


    g.setColour(juce::Colours::black);
    g.drawRect(0, 0, 667, 482, 2);
    g.drawRect(0, 480, 667,120, 2);
    g.drawRect(667, 0, 134, 600,2);
   

   
}

void OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    //auto sliderLeft = 50;
    gainSlider.setBounds(70, 500, 50, 200);

 

    auto bounds = getLocalBounds()/1.2;
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    
    responseCurveComponent.setBounds(responseArea);

    auto LowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto HighCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.5);

    lowCutFreqSlider.setBounds(LowCutArea.removeFromTop(HighCutArea.getHeight()*0.5));   
    lowCutSlopeSlider.setBounds(LowCutArea);

    highCutFreqSlider.setBounds(HighCutArea.removeFromTop(HighCutArea.getHeight() * 0.5));
    HighCutSlopeSlider.setBounds(HighCutArea);

    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
    driveKnob.setBounds(667, 20, 134, 134);
    rangeKnob.setBounds(667, 140, 134, 134);
    blendKnob.setBounds(667, 280, 134, 134);
    volumeKnob.setBounds(667, 420, 134, 134);



}



std::vector<juce::Component*> OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &HighCutSlopeSlider,
        &responseCurveComponent,
        &driveKnob,
        &rangeKnob,
        &blendKnob,
        &volumeKnob
    };

}



void OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &gainSlider)
    {
        audioProcessor.rawVolume = pow(10, gainSlider.getValue() / 20);
    }
}

