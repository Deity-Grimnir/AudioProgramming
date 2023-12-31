/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::OdinsSuperCoolAllPurposeAudioPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    //state = new juce::AudioProcessorValueTreeState(*this, nullptr);
}

OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::~OdinsSuperCoolAllPurposeAudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{

    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec{};

    spec.maximumBlockSize = samplesPerBlock;

    spec.numChannels = 1;

    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    auto chainSettings = getChainSettings(apvts);

    updatePeakFilter(chainSettings);

    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

    *leftChain.get<Chainpositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<Chainpositions::Peak>().coefficients = *peakCoefficients;

     auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,sampleRate,2*(chainSettings.lowCutSlope + 1));

   
    auto& leftLowCut = leftChain.get < Chainpositions::LowCut > ();
    updateCutFilter(leftLowCut, lowCutCoefficients,chainSettings.lowCutSlope);

    auto& rightLowCut = rightChain.get<Chainpositions::LowCut>();
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
      
    auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq, sampleRate, 2 * (chainSettings.highCutSlope + 1));

    auto& leftHighCut = leftChain.get <Chainpositions::HighCut>();
    auto& rightHighCut = rightChain.get <Chainpositions::HighCut>();

    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
    

//    leftChannelFifo.prepare(samplesPerBlock);
  //&  rightChannelFifo.prepare(samplesPerBlock);

}

void OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
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
#endif

void OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
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

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample<buffer.getNumSamples(); ++sample)
        {
            channelData[sample] = buffer.getSample(channel, sample) * rawVolume;
        }
    }

    updateFilters();
    juce::dsp::AudioBlock<float> block(buffer);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);

   // leftChannelFifo.update(buffer);
    //rightChannelFifo.update(buffer);


    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.


    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.



    auto chainSettings = getChainSettings(apvts);
    float drive = chainSettings.drive;
    float volume = chainSettings.volume;
    float range = chainSettings.range;
    float  blend = chainSettings.blend;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);


        // ..do something to the data...



        for (int sample = 0; sample < buffer.getNumSamples(); sample++)
        {


            float cleanSignal = *channelData;

            *channelData *= drive * range;

            *channelData = (((((2.f / juce::float_Pi) * atan(*channelData)) * blend) + (cleanSignal * (1.f - blend))) / 2.f) * rawVolume;

            channelData++;

        }

    }
}




float OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::applyDistortion(float inputSample, float drive)
{
    // Apply your distortion algorithm here and return the distorted sample
    // You can use the atan, tanh, or other functions for distortion

    // Example using tanh distortion
    return tanh(inputSample * drive);
}
//juce::AudioProcessorValueTreeState& OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::getState()
//{
//    return *state;
//}

//==============================================================================
bool OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::createEditor()
{
    return new OdinsSuperCoolAllPurposeAudioPluginAudioProcessorEditor (*this); 
    //return new juce::GenericAudioProcessorEditor(*this);

}

//==============================================================================
void OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

     juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);
        updateFilters();
    }


}
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());
    settings.drive = static_cast<float>(apvts.getRawParameterValue("Drive")->load());
    settings.range = static_cast<float>(apvts.getRawParameterValue("Range")->load());
    settings.blend = static_cast<float>(apvts.getRawParameterValue("Blend")->load());
    settings.volume = static_cast<float>(apvts.getRawParameterValue("Volume")->load());


    return settings;
}

Coefficients makePeakFilter(const ChainSettings& chainSettings,double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

}

void OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings)
{
    auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate());
    updateCoefficients(leftChain.get<Chainpositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<Chainpositions::Peak>().coefficients, peakCoefficients);
  

}

void updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
    *old = *replacements;
}
void OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings)
{
    auto cutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());
    auto& leftLowCut = leftChain.get<Chainpositions::LowCut >();
    auto& rightLowCut = rightChain.get<Chainpositions::LowCut>();
    //auto& distortionright = leftChain.get<Chainpositions::distortion>();
    //auto& distortionleft = rightChain.get<Chainpositions::distortion>();

    //updateCutFilter(distortionright, cutCoefficients, chainSettings.drive);
    //updateCutFilter(distortionleft, cutCoefficients, chainSettings.drive);
    updateCutFilter(rightLowCut, cutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(leftLowCut, cutCoefficients, chainSettings.lowCutSlope);
}

void OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{
    auto highCutCoefficients = makeHighcutFilter(chainSettings, getSampleRate());

    auto& leftHighCut = leftChain.get <Chainpositions::HighCut>();
    auto& rightHighCut = rightChain.get <Chainpositions::HighCut>();

    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}


void OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);
    updateLowCutFilters(chainSettings);
    updatePeakFilter(chainSettings);
    updateHighCutFilters(chainSettings);
}

void UpdateDistortion(const ChainSettings& chainSettings)
{



}
juce::AudioProcessorValueTreeState::ParameterLayout

OdinsSuperCoolAllPurposeAudioPluginAudioProcessor::createParamaterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq", 
                                                           "LowCut Freq",
                                                            juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                            20.f));
  
    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq", 
                                                           "HighCut Freq",
                                                            juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                            20000.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq", 
                                                           "Peak Freq",
                                                            juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                            750.0f));
   
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain", 
                                                           "Peak Gain",
                                                           juce::NormalisableRange<float>(-24.f, 24.f,0.5f, 0.25f),
                                                           0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality", 
                                                           "Peak Quality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f,0.05f, 0.25f),
                                                           1.f));
        layout.add(std::make_unique<juce::AudioParameterFloat>("Drive", 
                                                            "Drive",
                                                             juce::NormalisableRange<float>(0.f, 50.f, 0.5f, 0.2f),
                                                             0.f));
          layout.add(std::make_unique<juce::AudioParameterFloat>("Range", 
                                                            "Range",
                                                             juce::NormalisableRange<float>((0.f, 10.f, 0.5f), 1.f),
                                                             0.f));
                    layout.add(std::make_unique<juce::AudioParameterFloat>("Blend", 
                                                            "Blend",
                                                             juce::NormalisableRange<float>((0.01, 10.f, 0.001), 0.5f),
                                                             0.f));
                    layout.add(std::make_unique<juce::AudioParameterFloat>("Volume",
                        "Volume",
                        juce::NormalisableRange<float>((0.f, 10.f, 0), 1.f),
                        1.f));



    juce::StringArray stringArray;
        for (int i = 0; i < 4; ++i)
        {
            juce::String str;
            str << (12 + i * 12);
            str << " db/Oct";
            stringArray.add(str);
        }

        layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
        layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));


    return layout;
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OdinsSuperCoolAllPurposeAudioPluginAudioProcessor();
}

