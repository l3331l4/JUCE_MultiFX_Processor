/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <RotarySliderWithLabels.h>
#include <Utilities.h>

static juce::String getNameFromDSPOption(JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option option)
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

static JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option getDSPOptionFromName(const juce::String& name)
{
    if (name == "PHASE")
        return JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::Phase;
    else if (name == "CHORUS")
        return JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::Chorus;
    else if (name == "OVERDRIVE")
        return JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::Overdrive;
    else if (name == "LADDERFILTER")
        return JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::LadderFilter;
    else if (name == "GENFILTER")
        return JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::GeneralFilter;

    return JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::END_OF_LIST;
}

//==============================================================================
HorizontalConstrainer::HorizontalConstrainer(std::function<juce::Rectangle<int>()> confinerBoundsGetter, 
    std::function<juce::Rectangle<int>()> confineeBoundsGetter)
    : 
    boundsToConfineToGetter(std::move(confinerBoundsGetter)),
    boundsOfConfineeGetter(std::move(confineeBoundsGetter))
{

}

void HorizontalConstrainer::checkBounds(juce::Rectangle<int>& bounds,
    const juce::Rectangle<int>& previousBounds,
    const juce::Rectangle<int>& limits,
    bool isStretchingTop,
    bool isStretchingLeft,
    bool isStretchingBottom,
    bool isStretchingRight)
{
	bounds.setY(previousBounds.getY()); // Keep the Y position unchanged

    if (boundsToConfineToGetter != nullptr && boundsOfConfineeGetter != nullptr)
    {
        auto boundsToConfineTo = boundsToConfineToGetter();
        auto boundsOfConfinee = boundsOfConfineeGetter();

        bounds.setX(juce::jlimit(boundsToConfineTo.getX(), 
            boundsToConfineTo.getRight() - boundsOfConfinee.getWidth(), 
			bounds.getX()));
    }
    else
    {
        bounds.setX(juce::jlimit(limits.getX(),
            limits.getY(),bounds.getX()));
	}

	}

//==============================================================================
ExtendedTabBarButton::ExtendedTabBarButton(const juce::String& name, juce::TabbedButtonBar& owner, 
    JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option dspOption)
    : juce::TabBarButton(name, owner), option(dspOption)
{
    constrainer = std::make_unique<HorizontalConstrainer>(
        [&owner]() { return owner.getLocalBounds(); },
        [this]() { return getBounds(); }
	);

    constrainer->setMinimumOnscreenAmounts(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
}

int ExtendedTabBarButton::getBestTabLength(int depth)
{
	auto bestWidth = getLookAndFeel().getTabButtonBestWidth(*this, depth);
	auto& bar = getTabbedButtonBar();

	return juce::jmax(bestWidth, bar.getWidth() / bar.getNumTabs());
}

void ExtendedTabBarButton::mouseDown (const juce::MouseEvent& e)
{
    toFront(true);
    dragger.startDraggingComponent(this, e);
    juce::TabBarButton::mouseDown(e);
}

void ExtendedTabBarButton::mouseDrag(const juce::MouseEvent& e)
{
    dragger.dragComponent(this, e, constrainer.get());
}

//==============================================================================
ExtendedTabbedButtonBar::ExtendedTabbedButtonBar() : juce::TabbedButtonBar(juce::TabbedButtonBar::Orientation::TabsAtTop) 
{
	auto img = juce::Image(juce::Image::PixelFormat::SingleChannel, 1, 1, true);
	auto gfx = juce::Graphics(img);
	gfx.fillAll(juce::Colours::transparentBlack);

    dragImage = juce::ScaledImage(img, 1.0);
}

bool ExtendedTabbedButtonBar::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    if (dynamic_cast<ExtendedTabBarButton*>(dragSourceDetails.sourceComponent.get()))
        return true;
    return false;
}

void ExtendedTabbedButtonBar::itemDragEnter(const SourceDetails& dragSourceDetails)
{
    // This method is called when the mouse enters the tab bar while dragging.
    juce::DragAndDropTarget::itemDragEnter(dragSourceDetails);
}

struct Comparator
{

    static int compareElements(juce::TabBarButton* first, juce::TabBarButton* second)
    {
        if (first->getX() < second->getX())
            return -1;
        else if (first->getX() > second->getX())
			return 1;
		return 0; // They are equal in terms of X position
	}
};

juce::Array<juce::TabBarButton*> ExtendedTabbedButtonBar::getTabs()
{
    auto numTabs = getNumTabs();
    auto tabs = juce::Array<juce::TabBarButton*>();
    tabs.resize(numTabs);
    for (int i = 0; i < numTabs; ++i)
    {
        tabs.getReference(i) = getTabButton(i);
    }

    auto unsorted = tabs;
    Comparator comparator;
    tabs.sort(comparator);

    return tabs;
}

int ExtendedTabbedButtonBar::FindDraggedItemIndex(const SourceDetails& dragSourceDetails)
{
    if (auto tabButtonBeingDragged = dynamic_cast<ExtendedTabBarButton*>(dragSourceDetails.sourceComponent.get()))
    {
		auto tabs = getTabs();
		auto idx = tabs.indexOf(tabButtonBeingDragged);
        return idx;
    }
	return -1; // Not found
}

juce::TabBarButton* ExtendedTabbedButtonBar::findDraggedItem(const SourceDetails& dragSourceDetails)
{
	return getTabButton(FindDraggedItemIndex(dragSourceDetails));
}

void ExtendedTabbedButtonBar::itemDragMove(const SourceDetails& dragSourceDetails)
{
    if (auto tabButtonBeingDragged = dynamic_cast<ExtendedTabBarButton*>(dragSourceDetails.sourceComponent.get()))
    {

        auto idx = FindDraggedItemIndex(dragSourceDetails);
        if (idx == -1)
        {
            // The tab button being dragged is not in the current tab bar.
            jassertfalse;
            return;
        }

        auto previousTabIndex = idx - 1;
        auto nextTabIndex = idx + 1;
        auto previousTab = getTabButton(previousTabIndex);
        auto nextTab = getTabButton(nextTabIndex);

#define DEBUG_TAB_MOVEMENTS false
#if DEBUG_TAB_MOVEMENTS

        auto getButtonName = [](auto* button) -> juce::String 
            {
                if (button != nullptr)
                    return button->getButtonText();
                return "None";
			};
		juce::String prevName = getButtonName(previousTab);
		jassert(prevName.isNotEmpty());
		juce::String nextName = getButtonName(nextTab);
		jassert(nextName.isNotEmpty());
		DBG("ETBB::itemDragMove prev: [" << prevName << "] next: [" << nextName << "]");
#endif

		auto centreX = tabButtonBeingDragged->getBounds().getCentreX();

        if (previousTab == nullptr && nextTab != nullptr)
        {
            if (centreX > nextTab->getX() )
            {
				moveTab(idx, nextTabIndex);
            }
        }
        else if (previousTab != nullptr && nextTab == nullptr)
        {
            if (tabButtonBeingDragged->getX() < previousTab->getBounds().getCentreX())
            {
				moveTab(idx, previousTabIndex);
            }
        }
        else 
        {
            if (centreX > nextTab->getX())
            {
                moveTab(idx, nextTabIndex);
            }
            else if (centreX < previousTab->getRight())
            {
                moveTab(idx, previousTabIndex);
            }
        }

		tabButtonBeingDragged->toFront(true);
    }
}

void ExtendedTabbedButtonBar::itemDragExit(const SourceDetails& dragSourceDetails)
{
    // This method is called when the mouse exits the tab bar while dragging.
	juce::DragAndDropTarget::itemDragExit(dragSourceDetails);
}

void ExtendedTabbedButtonBar::itemDropped(const SourceDetails& dragSourceDetails) 
{
	resized();

	auto tabs = getTabs();
	JUCE_MultiFX_ProcessorAudioProcessor::DSP_Order newOrder;

	jassert(tabs.size() == newOrder.size());
    for (int i = 0; i < tabs.size(); ++i)
    {
		auto tab = tabs[static_cast<int>(i)];
        if (auto etbb = dynamic_cast<ExtendedTabBarButton*>(tab))
        {
			newOrder[i] = etbb->getOption();
        }
    }

    listeners.call([newOrder](Listener& l) {
		l.tabOrderChanged(newOrder);
        });

}

void ExtendedTabbedButtonBar::mouseDown(const juce::MouseEvent& e)
{
    if (auto tabButtonBeingDragged = dynamic_cast<ExtendedTabBarButton*>(e.originalComponent))
    {

        auto tabs = getTabs();
		auto idx = tabs.indexOf(tabButtonBeingDragged);
        if (idx != -1) 
        {
			setCurrentTabIndex(idx);
        }

		startDragging(tabButtonBeingDragged->TabBarButton::getTitle(),
            tabButtonBeingDragged, dragImage);
    }
}

juce::TabBarButton* ExtendedTabbedButtonBar::createTabButton(const juce::String& tabName, int tabIndex)
{
	auto dspOption = getDSPOptionFromName(tabName);
	auto etbb = std::make_unique<ExtendedTabBarButton>(tabName, *this, dspOption);
	etbb->addMouseListener(this, false);
	return etbb.release();
}

void ExtendedTabbedButtonBar::addListener(Listener* l)
{
    listeners.add(l);
} 

void ExtendedTabbedButtonBar::removeListener(Listener* l)
{
    listeners.remove(l);
}

void ExtendedTabbedButtonBar::currentTabChanged(int newCurrentTabIndex, const juce::String& newCurrentTabName)
{
	juce::ignoreUnused(newCurrentTabName);
    listeners.call([newCurrentTabIndex](Listener& l) {
        l.selectedTabChanged(newCurrentTabIndex);
		});
}

//==============================================================================
DSP_Gui::DSP_Gui(JUCE_MultiFX_ProcessorAudioProcessor& proc)
    : processor(proc)
{
}



void DSP_Gui::resized()  
{
	auto bounds = getLocalBounds();
    if (buttons.empty() == false)
    {
		auto buttonArea = bounds.removeFromTop(30);
		auto w = buttonArea.getWidth() / buttons.size();
        for (auto& button : buttons)
        {
			button->setBounds(buttonArea.removeFromLeft(static_cast<int>(w)));
        }
    }

    if (comboBoxes.empty() == false)
    {
        auto comboBoxArea = bounds.removeFromLeft(bounds.getWidth() / 4);
        auto h = juce::jmin(comboBoxArea.getHeight() / static_cast<int>(comboBoxes.size()), 30);
        for (auto& cb : comboBoxes)
        {
            cb->setBounds(comboBoxArea.removeFromTop(static_cast<int>(h)));
        }
    }

    if (sliders.empty() == false)
    {
        auto w = bounds.getWidth() / sliders.size();
        for (auto& slider : sliders)
        {
            slider->setBounds(bounds.removeFromLeft(static_cast<int>(w)));
        }
    }
}

void DSP_Gui::paint(juce::Graphics& g)
{
    //g.fillAll(juce::Colours::black);
    g.fillAll(juce::Colour(233, 236, 241));
}

void DSP_Gui::rebuildInterface(std::vector< juce::RangedAudioParameter* > params)
{
    if (params == currentParams)
    {
		// No need to rebuild if the parameters haven't changed
		DBG("INTERFACE REBUILD SKIPPED: No changes in parameters");
        return;
    }

	// Interface rebuild
	currentParams = params;

	sliderAttachments.clear();
	comboBoxAttachments.clear();
	buttonAttachments.clear();

	sliders.clear();
	comboBoxes.clear();
	buttons.clear();

    for (size_t i = 0; i < params.size(); ++i)
    {
		auto p = params[i];

		if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(p)) // Choice parameters
        {
			comboBoxes.push_back(std::make_unique<juce::ComboBox>());
			auto& cb = *comboBoxes.back();
			cb.addItemList(choice->choices, 1);
            comboBoxAttachments.push_back(
                std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
                (processor.apvts, p->getName(100), cb));
        }
		else if (auto* toggle = dynamic_cast<juce::AudioParameterBool*>(p)) // Toggle parameters
        {
			buttons.push_back(std::make_unique<juce::ToggleButton>("Bypass"));
			auto& btn = *buttons.back();
            buttonAttachments.push_back(
                std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
				(processor.apvts, p->getName(100), btn));
        }
		else // Slider parameters
        {
            sliders.push_back(std::make_unique<RotarySliderWithLabels>(p, p->label, p->getName(100)));
            auto& slider = *sliders.back();
            SimpleMBComp::addLabelPairs(slider.labels, *p, p->label);
			slider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
            sliderAttachments.push_back(
				std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
				(processor.apvts, p->getName(100), slider));
        }
    }

    for (auto& slider : sliders)
    {
		addAndMakeVisible(slider.get());
    }
    for (auto& cb : comboBoxes)
    {
		addAndMakeVisible(cb.get());
    }
    for (auto& btn : buttons)
    {
        addAndMakeVisible(btn.get());
    }

	resized();
       
}

//==============================================================================
JUCE_MultiFX_ProcessorAudioProcessorEditor::JUCE_MultiFX_ProcessorAudioProcessorEditor (JUCE_MultiFX_ProcessorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setLookAndFeel(&lookAndFeel);
	addAndMakeVisible(tabbedComponent);
	addAndMakeVisible(dspGUI);

	audioProcessor.guiNeedsLatestDspOrder.set(true);

	tabbedComponent.addListener(this);
	startTimerHz(30); // Timer to update the UI
    setSize (768, 400);
}

JUCE_MultiFX_ProcessorAudioProcessorEditor::~JUCE_MultiFX_ProcessorAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
	tabbedComponent.removeListener(this);
}

//==============================================================================
void JUCE_MultiFX_ProcessorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    auto fillMeter = [&](auto rect, const auto& rmsSource)
        {
			g.setColour(juce::Colours::black);
			g.fillRect(rect);

			auto rms = rmsSource.get();
            if (rms > 1.f)
            {
				g.setColour(juce::Colours::red);
                auto lowerLeft = juce::Point<float>(rect.getX(),
                    juce::jmap<float>(juce::Decibels::gainToDecibels(1.f),
                        NEGATIVE_INFINITY, MAX_DECIBELS, rect.getBottom(), rect.getY()));

                auto upperRight = juce::Point<float>(rect.getRight(),
                    juce::jmap<float>(juce::Decibels::gainToDecibels(rms),
						NEGATIVE_INFINITY, MAX_DECIBELS, rect.getBottom(), rect.getY()));

				auto overThreshRect = juce::Rectangle<float>(lowerLeft, upperRight);

				g.fillRect(overThreshRect);
            }

            rms = juce::jmin(rms, 1.f);
			g.setColour(juce::Colours::green);
            g.fillRect(rect.withY(juce::jmap<float>(juce::Decibels::gainToDecibels(rms),
					NEGATIVE_INFINITY, MAX_DECIBELS, rect.getBottom(), rect.getY())).withBottom(rect.getBottom()));
        };

	

    auto drawTicks = [&](auto rect, auto leftMeterRightEdge, auto rightMeterLeftEdge)
        {
            for (int i = MAX_DECIBELS; i >= NEGATIVE_INFINITY; i -= 12)
            {
				auto y = juce::jmap<int>(i, NEGATIVE_INFINITY, MAX_DECIBELS, rect.getBottom(), rect.getY());
                auto r = juce::Rectangle<int>(rect.getWidth(), fontHeight);
                r.setCentre(rect.getCentreX(), y);

				g.setColour(i == 0 ? juce::Colours::black : 
                    i > 0 ? juce::Colours::red : 
                juce::Colours::lightsteelblue);

				g.drawFittedText(juce::String(i), r, juce::Justification::centred, 1);

                if (i != MAX_DECIBELS && i != NEGATIVE_INFINITY)
                {
                    g.drawLine(rect.getX() + tickIndent, y, leftMeterRightEdge - tickIndent, y);
					g.drawLine(rightMeterLeftEdge + tickIndent, y, rect.getRight() - tickIndent, y);
                }
            }
        };

    auto drawMeter = [&fillMeter, &drawTicks](juce::Rectangle<int> rect, 
        juce::Graphics& g, const juce::Atomic<float>& leftSource, 
        const juce::Atomic<float>& rightSource,
        const auto& label)
        {
			g.setColour(juce::Colours::green);
			g.drawRect(rect);
            rect.reduce(2, 2);

			g.setColour(juce::Colours::black);
			g.drawText(label, rect.removeFromBottom(fontHeight), juce::Justification::centred);

			rect.removeFromTop(fontHeight / 2);
            const auto meterArea = rect;
			const auto leftChan = rect.removeFromLeft(meterChanWidth);
			const auto rightChan = rect.removeFromRight(meterChanWidth);

			fillMeter(leftChan, leftSource);
			fillMeter(rightChan, rightSource);

            drawTicks(meterArea, 
				leftChan.getRight(), rightChan.getX());


        };

    auto bounds = getLocalBounds();
    auto preMeterArea = bounds.removeFromLeft(meterWidth);
    auto postMeterArea = bounds.removeFromRight(meterWidth);

    drawMeter(preMeterArea, g, 
        audioProcessor.leftPreRMS, audioProcessor.rightPreRMS,
		"In");

    drawMeter(postMeterArea, g,
        audioProcessor.leftPostRMS, audioProcessor.rightPostRMS,
        "Out");
	
}

void JUCE_MultiFX_ProcessorAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
	auto bounds = getLocalBounds();
	auto leftMeterArea = bounds.removeFromLeft(meterWidth);
	auto rightMeterArea = bounds.removeFromRight(meterWidth);
	juce::ignoreUnused(leftMeterArea, rightMeterArea);

    bounds.removeFromTop(10);
    tabbedComponent.setBounds(bounds.removeFromTop(30));
	dspGUI.setBounds(bounds);

}

void JUCE_MultiFX_ProcessorAudioProcessorEditor::tabOrderChanged(JUCE_MultiFX_ProcessorAudioProcessor::DSP_Order newOrder)
{
    rebuildInterface();
	audioProcessor.dspOrderFifo.push(newOrder);
}

void JUCE_MultiFX_ProcessorAudioProcessorEditor::timerCallback()
{
    
	repaint();

    if (audioProcessor.restoreDspOrderFifo.getNumAvailableForReading() == 0)
        return;

	using T = JUCE_MultiFX_ProcessorAudioProcessor::DSP_Order;
	T newOrder;
	newOrder.fill(JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::END_OF_LIST);
    auto empty = newOrder;
    while (audioProcessor.restoreDspOrderFifo.pull(newOrder))
    {
        ;
    }

    if (newOrder != empty)
    {
		addTabsFromDSPOrder(newOrder);
	}

    if (selectedTabAttachment == nullptr)
    {
        selectedTabAttachment = std::make_unique<juce::ParameterAttachment>(*audioProcessor.selectedTab,[this](float tabNum)
            {
				auto newTabNum = static_cast<int>(tabNum);
                if (juce::isPositiveAndBelow(newTabNum, tabbedComponent.getNumTabs()))
                {
					tabbedComponent.setCurrentTabIndex(newTabNum);
                }
                else
                {
					jassertfalse; // Invalid tab index
                }
            });
		selectedTabAttachment->sendInitialUpdate();
    }
}

void JUCE_MultiFX_ProcessorAudioProcessorEditor::addTabsFromDSPOrder(JUCE_MultiFX_ProcessorAudioProcessor::DSP_Order newOrder)
{
    tabbedComponent.clearTabs();
    for (auto v : newOrder)
    {
		tabbedComponent.addTab(getNameFromDSPOption(v), juce::Colours::white, -1);
	}

    rebuildInterface();
	audioProcessor.dspOrderFifo.push(newOrder);
}   

void JUCE_MultiFX_ProcessorAudioProcessorEditor::rebuildInterface()
{
	auto currentTabIndex = tabbedComponent.getCurrentTabIndex();
	auto currentTab = tabbedComponent.getTabButton(currentTabIndex);
    if (auto etbb = dynamic_cast<ExtendedTabBarButton*>(currentTab))
    {
		auto option = etbb->getOption();
		auto params = audioProcessor.getParamsForOption(option);
		jassert(params.empty() == false); // Ensure we have parameters for the selected DSP option
		dspGUI.rebuildInterface(params);
    }
}

void JUCE_MultiFX_ProcessorAudioProcessorEditor::selectedTabChanged(int newCurrentTabIndex)
{
    if ( selectedTabAttachment )
    {
        rebuildInterface();
		selectedTabAttachment->setValueAsCompleteGesture(static_cast<float>(newCurrentTabIndex));
		
	}
}