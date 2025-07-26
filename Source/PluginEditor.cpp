/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::String getDSPOptionName(JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option option)
{
    switch (option)
    {
    case JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::Phase:
        return "PHASE";
    case JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::Chorus:
        return "CHORUS";
    case JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::Overdrive:
        return "OVERDRIVE";
    case JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::LadderFilter:
        return "LADDERFILTER";
    case JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::GeneralFilter:
        return "GENFILTER";
    case JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::END_OF_LIST:
        jassertfalse;
    }

	return "NO SELECTION";
}

ExtendedTabBarButton::ExtendedTabBarButton(const juce::String& name, juce::TabbedButtonBar& owner)
    : juce::TabBarButton(name, owner)
{
}

juce::TabBarButton* ExtendedTabbedButtonBar::createTabButton(const juce::String& tabName, int tabIndex)
{
	return new ExtendedTabBarButton(tabName, *this);
}

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

			tabbedComponent.clearTabs();

            for (auto& option : dspOrder)
            {
				auto entry = random.nextInt(range);
				option = static_cast<JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option>(entry);
				tabbedComponent.addTab(getDSPOptionName(option), juce::Colours::white, -1);
            }
			DBG(juce::Base64::toBase64(dspOrder.data(), dspOrder.size()));

			audioProcessor.dspOrderFifo.push(dspOrder);

		};


	addAndMakeVisible(dspOrderButton);
	addAndMakeVisible(tabbedComponent);
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
	auto bounds = getLocalBounds();
	dspOrderButton.setBounds(bounds.removeFromTop(30).withSizeKeepingCentre(150,30));
    bounds.removeFromTop(10);
    tabbedComponent.setBounds(bounds.withHeight(30));

}
