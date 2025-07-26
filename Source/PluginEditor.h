/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct ExtendedTabbedButtonBar : juce::TabbedButtonBar, juce::DragAndDropTarget
{
    ExtendedTabbedButtonBar() : juce::TabbedButtonBar(juce::TabbedButtonBar::Orientation::TabsAtTop) {}
    
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override
    {
        return false;
	}

    void itemDropped (const SourceDetails& dragSourceDetails) override
    {
        // Handle the drop event here
	}

    juce::TabBarButton* createTabButton(const juce::String& tabName, int tabIndex) override;
};

struct ExtendedTabBarButton : juce::TabBarButton
{
    ExtendedTabBarButton(const juce::String& name, juce::TabbedButtonBar& owner);
	juce::ComponentDragger dragger;

    void mouseDown (const juce::MouseEvent& e)
    {
        dragger.startDraggingComponent(this, e);
	}

    void mouseDrag (const juce::MouseEvent& e)
    {
        dragger.dragComponent(this, e, nullptr);
    }

};


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

	ExtendedTabbedButtonBar tabbedComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JUCE_MultiFX_ProcessorAudioProcessorEditor)
};
