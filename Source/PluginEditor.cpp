/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
JUCE_MultiFX_ProcessorAudioProcessorEditor::JUCE_MultiFX_ProcessorAudioProcessorEditor (JUCE_MultiFX_ProcessorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    dspOrderButton.onClick = [this]()
    {
			juce::Random random;
			JUCE_MultiFX_ProcessorAudioProcessor::DSP_Order dspOrder;

			auto range = juce::Range<int>(static_cast<int>(JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::Phase), 
                static_cast<int>(JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::END_OF_LIST));
            for (auto& option : dspOrder)
            {
				auto entry = random.nextInt(range);
				option = static_cast<JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option>(random.nextInt(range));
            }
			DBG(juce::Base64::toBase64(dspOrder.data(), dspOrder.size()));
            jassertfalse;

			audioProcessor.dspOrderFifo.push(dspOrder);

		};


	addAndMakeVisible(dspOrderButton);
    setSize (400, 300);
}

JUCE_MultiFX_ProcessorAudioProcessorEditor::~JUCE_MultiFX_ProcessorAudioProcessorEditor()
{
}

//==============================================================================
void JUCE_MultiFX_ProcessorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void JUCE_MultiFX_ProcessorAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

	dspOrderButton.setBounds(getLocalBounds().reduced(100));
}
