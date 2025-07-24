/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class JUCE_MultiFX_ProcessorAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    JUCE_MultiFX_ProcessorAudioProcessorEditor (JUCE_MultiFX_ProcessorAudioProcessor&);
    ~JUCE_MultiFX_ProcessorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    JUCE_MultiFX_ProcessorAudioProcessor& audioProcessor;

	juce::TextButton dspOrderButton{ "Change DSP Order" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JUCE_MultiFX_ProcessorAudioProcessorEditor)
};
