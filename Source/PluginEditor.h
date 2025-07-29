/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct ExtendedTabbedButtonBar : juce::TabbedButtonBar, juce::DragAndDropTarget, juce::DragAndDropContainer
{
    ExtendedTabbedButtonBar();
    
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;

    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;


    void mouseDown(const juce::MouseEvent& e) override;

    juce::TabBarButton* createTabButton(const juce::String& tabName, int tabIndex) override;

    struct Listener
    {
		virtual ~Listener() = default;
		virtual void tabOrderChanged(JUCE_MultiFX_ProcessorAudioProcessor::DSP_Order newOrder) = 0;
    };

    void addListener(Listener* l);
	void removeListener(Listener* l);

private:
    juce::TabBarButton* findDraggedItem(const SourceDetails& dragSourceDetails);
	int FindDraggedItemIndex(const SourceDetails& dragSourceDetails);
	juce::Array<juce::TabBarButton*> getTabs();

	juce::ScaledImage dragImage;
	juce::ListenerList<Listener> listeners;
};

struct HorizontalConstrainer : juce::ComponentBoundsConstrainer
{
    HorizontalConstrainer(std::function<juce::Rectangle<int>()> confinerBoundsGetter,
        std::function<juce::Rectangle<int>()> confineeBoundsGetter);

    void checkBounds(juce::Rectangle<int>& bounds,
        const juce::Rectangle<int>& previousBounds,
        const juce::Rectangle<int>& limits,
        bool isStretchingTop,
        bool isStretchingLeft,
        bool isStretchingBottom,
        bool isStretchingRight) override;

private: 
    std::function<juce::Rectangle<int>()> boundsToConfineToGetter;
	std::function<juce::Rectangle<int>()> boundsOfConfineeGetter;
};

struct ExtendedTabBarButton : juce::TabBarButton
{
    ExtendedTabBarButton(const juce::String& name, juce::TabbedButtonBar& owner, JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option dspOption);
	juce::ComponentDragger dragger;
	std::unique_ptr<HorizontalConstrainer> constrainer;

    void mouseDown(const juce::MouseEvent& e) override;

    void mouseDrag(const juce::MouseEvent& e) override;

    JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option getOption() const { return option; }

    int getBestTabLength(int depth) override;

private:
    JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option option;

};

struct DSP_Gui : juce::Component
{
    DSP_Gui() {}

    void resized() override {}
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::blue);
    }

    void rebuildInterface(std::vector< juce::RangedAudioParameter* > params);

    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
};


//==============================================================================
/**
*/
class JUCE_MultiFX_ProcessorAudioProcessorEditor  : public juce::AudioProcessorEditor,
	ExtendedTabbedButtonBar::Listener,
    juce::Timer
{
public:
    JUCE_MultiFX_ProcessorAudioProcessorEditor (JUCE_MultiFX_ProcessorAudioProcessor&);
    ~JUCE_MultiFX_ProcessorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

	void tabOrderChanged(JUCE_MultiFX_ProcessorAudioProcessor::DSP_Order newOrder) override;
	void timerCallback() override;
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    JUCE_MultiFX_ProcessorAudioProcessor& audioProcessor;
	DSP_Gui dspGUI;
	ExtendedTabbedButtonBar tabbedComponent;

    void addTabsFromDSPOrder(JUCE_MultiFX_ProcessorAudioProcessor::DSP_Order dspOrder);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JUCE_MultiFX_ProcessorAudioProcessorEditor)
};
