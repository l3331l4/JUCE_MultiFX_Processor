/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
JUCE_MultiFX_ProcessorAudioProcessor::JUCE_MultiFX_ProcessorAudioProcessor()
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
}

JUCE_MultiFX_ProcessorAudioProcessor::~JUCE_MultiFX_ProcessorAudioProcessor()
{
}

//==============================================================================
const juce::String JUCE_MultiFX_ProcessorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool JUCE_MultiFX_ProcessorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool JUCE_MultiFX_ProcessorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool JUCE_MultiFX_ProcessorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double JUCE_MultiFX_ProcessorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int JUCE_MultiFX_ProcessorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int JUCE_MultiFX_ProcessorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void JUCE_MultiFX_ProcessorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String JUCE_MultiFX_ProcessorAudioProcessor::getProgramName (int index)
{
    return {};
}

void JUCE_MultiFX_ProcessorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void JUCE_MultiFX_ProcessorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void JUCE_MultiFX_ProcessorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool JUCE_MultiFX_ProcessorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

juce::AudioProcessorValueTreeState::ParameterLayout JUCE_MultiFX_ProcessorAudioProcessor::createParameterlayout() {

	juce::AudioProcessorValueTreeState::ParameterLayout layout;
	return layout;
}

void JUCE_MultiFX_ProcessorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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

	// TODO: create audio parameters for all DSP options
	// TODO: update the DSP chain based on the parameters
    // TODO: save/load settings
	// TODO: save/load DSP order
	// TODO: drag to reorder GUI
	// TODO: GUI design for each DSP option
    // TODO: metering
	// TODO: prepare all DSP options
	// TODO: wet/dry mix control [STRETCH]
	// TODO: mono and stereo versions [STRETCH]
	// TODO: modulators (eg. LFOs, envelopes, etc.) [STRETCH]
	// TODO: thread-safe filter updates [STRETCH]
	// TODO: pre/post filtering [STRETCH]
	// TODO: delay module [STRETCH]

    auto newDSPOrder = DSP_Order();

	// Try to pull the DSP order from the FIFO
    while (dspOrderFifo.pull(newDSPOrder))
    {

    }

	// If the DSP order has changed, we need to reconfigure the DSP chain
    if ( newDSPOrder != DSP_Order() )
		dspOrder = newDSPOrder;

	// Convert DSP_Order to DSP_Pointers
	DSP_Pointers dspPointers;

    for( size_t i = 0; i < dspPointers.size(); ++i )
    {
        switch (dspOrder[i])
        {
            case DSP_Option::Phase:
                dspPointers[i] = &phaser;
                break;
            case DSP_Option::Chorus:
                dspPointers[i] = &chorus;
                break;
            case DSP_Option::Overdrive:
                dspPointers[i] = &overdrive;
                break;
            case DSP_Option::LadderFilter:
                dspPointers[i] = &ladderFilter;
                break;
			case DSP_Option::END_OF_LIST:
				jassertfalse; // This should never happen
                break;
        }
	}

	// Process the audio through the DSP chain
	auto block = juce::dsp::AudioBlock<float>(buffer);
    auto context = juce::dsp::ProcessContextReplacing<float>(block);

    for (size_t i = 0; i < dspPointers.size(); ++i)
    {
        if (dspPointers[i] != nullptr)
        {
            dspPointers[i]->process(context);
		}
    }

}

//==============================================================================
bool JUCE_MultiFX_ProcessorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* JUCE_MultiFX_ProcessorAudioProcessor::createEditor()
{
    return new JUCE_MultiFX_ProcessorAudioProcessorEditor (*this);
}

//==============================================================================
void JUCE_MultiFX_ProcessorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void JUCE_MultiFX_ProcessorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JUCE_MultiFX_ProcessorAudioProcessor();
}
