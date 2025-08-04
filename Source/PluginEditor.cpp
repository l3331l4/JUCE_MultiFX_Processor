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
            limits.getRight() - bounds.getWidth(),
            bounds.getX()));
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
    auto& bar = getTabbedButtonBar();
    auto totalWidth = bar.getWidth();
    auto numTabs = bar.getNumTabs();
    auto overlap = getLookAndFeel().getTabButtonOverlap(depth);

    auto buttonWidth = (totalWidth + (overlap * (numTabs - 1))) / numTabs;

    if (getIndex() == numTabs - 1)
    {
        auto calculatedTotalWidth = (buttonWidth * (numTabs - 1)) - (overlap * (numTabs - 1));
        auto remainingWidth = totalWidth - calculatedTotalWidth;
        return remainingWidth;
    }

    return buttonWidth;
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
    auto internalTabs = juce::Array<juce::TabBarButton*>();
    internalTabs.resize(numTabs);
    for (int i = 0; i < numTabs; ++i)
    {
        internalTabs.getReference(i) = getTabButton(i);
    }

    //auto unsorted = internalTabs;
    //Comparator comparator;
    //internalTabs.sort(comparator);

    return internalTabs;
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

void ExtendedTabbedButtonBar::itemDragMove(const SourceDetails &dragSourceDetails)
{
    if (auto tabButtonBeingDragged = dynamic_cast<ExtendedTabBarButton*>(dragSourceDetails.sourceComponent.get()))
    {

		auto idx = tabs.indexOf(tabButtonBeingDragged);
        if (idx == -1)
        {
            // The tab button being dragged is not in the current tab bar.
            jassertfalse;
            return;
        }

        auto previousTabIndex = idx - 1;
        auto nextTabIndex = idx + 1;
		auto previousTab = juce::isPositiveAndBelow(previousTabIndex, tabs.size()) ? tabs[previousTabIndex] : nullptr;
        auto nextTab = juce::isPositiveAndBelow(nextTabIndex, tabs.size()) ? tabs[nextTabIndex] : nullptr;

		auto centreX = tabButtonBeingDragged->getBounds().getCentreX();

        if (centreX > previousDraggedTabCenterPosition.x)
        {
			// The tab button is being dragged to the right.
            if (nextTab != nullptr)
            {
                if (previousDraggedTabCenterPosition.x < nextTab->getX() && nextTab->getX() <= centreX)
                {
					DBG("Swapping " << tabButtonBeingDragged->getName() << " with " << nextTab->getName());
                    nextTab->setBounds(nextTab->getBounds().withX(previousTab != nullptr ? previousTab->getRight() : 0));
					tabs.swap(idx, nextTabIndex);
                }
            }
        }
        else if (centreX < previousDraggedTabCenterPosition.x)
        {
            // The tab button is being dragged to the left.
            if (previousTab != nullptr)
            {
                if (previousDraggedTabCenterPosition.x > previousTab->getRight() && centreX <= previousTab->getRight())
                {
					DBG("Swapping " << tabButtonBeingDragged->getName() << " with " << previousTab->getName());
					previousTab->setBounds(previousTab->getBounds().withX(nextTab != nullptr ? 
                        nextTab->getX() - previousTab->getWidth(): getWidth() - previousTab->getWidth()));
					tabs.swap(idx, previousTabIndex);
                }
            }
        }

		tabButtonBeingDragged->toFront(true);

		previousDraggedTabCenterPosition = tabButtonBeingDragged->getBounds().getCentre();
    }
}

bool ExtendedTabbedButtonBar::reorderTabsAfterDrop()
{
	bool tabOrderChanged = false;

#define DEBUG_TAB_ORDER true

    while (true)
    {
		auto internalTabs = getTabs();
        if (internalTabs == tabs)
            break; // No changes in the tab order
        
        for (int i = 0; i < tabs.size(); ++i)
        {
			auto t = tabs[i];
			auto location = internalTabs.indexOf(t);
            if (i != location)
            {
#if DEBUG_TAB_ORDER
                DBG("Starting tab order:");
                for (auto& tab : tabs)
                {
                    DBG("Tab: " << tab->getName() << " at index: " << tabs.indexOf(tab));
                }
#endif
				moveTab(location, i);
				tabOrderChanged = true;
                break;
            }
        }
    }
	return tabOrderChanged;
}

void ExtendedTabbedButtonBar::itemDragExit(const SourceDetails& dragSourceDetails)
{
    // This method is called when the mouse exits the tab bar while dragging.
	juce::DragAndDropTarget::itemDragExit(dragSourceDetails);
}

void ExtendedTabbedButtonBar::itemDropped(const SourceDetails& dragSourceDetails) 
{
    if (reorderTabsAfterDrop() == false)
    {
		// If no reordering was done, but just a drag and drop operation, we lock the tab position of the dragged tab.
        resized();
    }

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

        tabs = getTabs();
		auto idx = tabs.indexOf(tabButtonBeingDragged);
        if (idx != -1) 
        {
			setCurrentTabIndex(idx);
			setTabColours();
        }

		startDragging(tabButtonBeingDragged->TabBarButton::getTitle(),
            tabButtonBeingDragged, dragImage);
    }
}

void ExtendedTabbedButtonBar::setTabColours()
{
	auto tabs = getTabs();

    for (int i = 0; i < tabs.size(); ++i)
    {
        auto colour = tabs[i]->isFrontTab() ? ColorScheme::getBackgroundColor() : // Background color for active tab
			ColorScheme::getInactiveTabColor(); // Default colour for inactive tabs

        setTabBackgroundColour(i, colour);
        tabs[i]->repaint();
	}
}

PowerButtonWithParam::PowerButtonWithParam(juce::AudioParameterBool* p)
{
    jassert(p != nullptr);
    changeAttachment(p);
}

void PowerButtonWithParam::changeAttachment(juce::AudioParameterBool* p)
{
    attachment.reset();
    if (p != nullptr)
    {
        param = p;
        attachment = std::make_unique<juce::ButtonParameterAttachment>(*p, *this);
        attachment->sendInitialUpdate();
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
    g.fillAll(ColorScheme::getBackgroundColor());
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

        if (dynamic_cast<juce::AudioParameterBool*>(p))
        {
            DBG("Skipping button attachment");
        }
        else
        {
            sliders.push_back(std::make_unique<RotarySliderWithLabels>(p, p->label, p->getName(100)));
            auto& slider = *sliders.back();
            SimpleMBComp::addLabelPairs(slider.labels, *p, p->label);
            slider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
            sliderAttachments.push_back(
                std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                (processor.apvts, p->getName(100), slider));
        }

#if false
		if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(p)) // Choice parameters
        {
			/*comboBoxes.push_back(std::make_unique<juce::ComboBox>());
			auto& cb = *comboBoxes.back();
			cb.addItemList(choice->choices, 1);
            comboBoxAttachments.push_back(
                std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
                (processor.apvts, p->getName(100), cb));*/
        }
		else if (auto* toggle = dynamic_cast<juce::AudioParameterBool*>(p)) // Toggle parameters
        {
			// Creating a power button for toggling

			/*buttons.push_back(std::make_unique<juce::ToggleButton>("Bypass"));
			auto& btn = *buttons.back();
            buttonAttachments.push_back(
                std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
				(processor.apvts, p->getName(100), btn));*/
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
#endif
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

void DSP_Gui::toggleSliderEnablement(bool enabled)
{
    for (auto& slider : sliders)
		slider->setEnabled(enabled);
    for (auto& cb : comboBoxes)
		cb->setEnabled(enabled);
    for (auto& btn : buttons)
		btn->setEnabled(enabled);
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

	addAndMakeVisible(analyzer);

    inGainControl = std::make_unique<RotarySliderWithLabels>(
		audioProcessor.inputGain, "dB", "IN");

	outGainControl = std::make_unique<RotarySliderWithLabels>(
        audioProcessor.outputGain, "dB", "OUT");

	addAndMakeVisible(inGainControl.get());
	addAndMakeVisible(outGainControl.get());

	SimpleMBComp::addLabelPairs(inGainControl->labels, *audioProcessor.inputGain, "dB");
	SimpleMBComp::addLabelPairs(outGainControl->labels, *audioProcessor.outputGain, "dB");

    inGainAttachment = std::make_unique<juce::SliderParameterAttachment>(
		*audioProcessor.inputGain, *inGainControl);

	outGainAttachment = std::make_unique<juce::SliderParameterAttachment>(
        *audioProcessor.outputGain, *outGainControl);

	audioProcessor.guiNeedsLatestDspOrder.set(true);

	tabbedComponent.addListener(this);
	startTimerHz(30); // Timer to update the UI

	float scaleFactor = 1.2f;
    setSize (768 * scaleFactor, 450 * scaleFactor);
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
			g.setColour(ColorScheme::getTitleColor());
			g.fillRect(rect);

			auto rms = rmsSource.get();
            if (rms > 1.f)
            {
				g.setColour(ColorScheme::getIndustrialRed());
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
            juce::Font meterFont = lookAndFeel.getIBMPlexMonoMediumFont(static_cast<float>(fontHeight) * 0.7f);
            g.setFont(meterFont);

            for (int i = MAX_DECIBELS; i >= NEGATIVE_INFINITY; i -= 12)
            {
				auto y = juce::jmap<int>(i, NEGATIVE_INFINITY, MAX_DECIBELS, rect.getBottom(), rect.getY());
                auto r = juce::Rectangle<int>(rect.getWidth(), fontHeight);
                r.setCentre(rect.getCentreX(), y);

				g.setColour(i == 0 ? ColorScheme::getTitleColor() :
                    i > 0 ? ColorScheme::getIndustrialRed() :
                    ColorScheme::getTitleColor());

				g.drawFittedText(juce::String(i), r, juce::Justification::centred, 1);

				g.setColour(ColorScheme::getBackgroundColor());

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
            rect.reduce(5, 2);

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
	auto ioArea = bounds.removeFromBottom(ioControlSize);

    auto preMeterArea = bounds.removeFromLeft(meterWidth);
    auto postMeterArea = bounds.removeFromRight(meterWidth);

    juce::Colour ioBgColor = ColorScheme::getBackgroundColor();
    g.setColour(ioBgColor);

    g.fillRect(preMeterArea);
    g.fillRect(postMeterArea);
    g.fillRect(ioArea);

    drawMeter(preMeterArea, g, 
        audioProcessor.leftPreRMS, audioProcessor.rightPreRMS,
		"");

    drawMeter(postMeterArea, g,
        audioProcessor.leftPostRMS, audioProcessor.rightPostRMS,
        "");
	
}

void JUCE_MultiFX_ProcessorAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
	auto bounds = getLocalBounds();
	/*auto gainArea = bounds.removeFromBottom(ioControlSize);
	inGainControl->setBounds(gainArea.removeFromLeft(ioControlSize));
	outGainControl->setBounds(gainArea.removeFromRight(ioControlSize));*/

	auto leftMeterArea = bounds.removeFromLeft(meterWidth);
	auto rightMeterArea = bounds.removeFromRight(meterWidth);
	inGainControl->setBounds(leftMeterArea.removeFromBottom(ioControlSize));
    outGainControl->setBounds(rightMeterArea.removeFromBottom(ioControlSize));

	analyzer.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.7));

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
		tabbedComponent.addTab(getNameFromDSPOption(v), ColorScheme::getTitleColor(), -1);

	}

	auto numTabs = tabbedComponent.getNumTabs();
	auto size = tabbedComponent.getHeight();
    for (int i = 0; i < numTabs; ++i)
    {
        if (auto tab = tabbedComponent.getTabButton(i))
        {
            auto order = newOrder[i];
            auto params = audioProcessor.getParamsForOption(order);

            if (auto bypass = findBypassParam(params))
            {
                auto pbwp = std::make_unique<PowerButtonWithParam>(bypass);
                pbwp->setSize(size, size);
                

                pbwp->onClick = [this, btn = pbwp.get()]()
                {
                    auto idx = tabbedComponent.getCurrentTabIndex();
                    if (auto tabButton = tabbedComponent.getTabButton(idx))
                    {
                        if (tabButton->getExtraComponent() == btn)
                        {
							refreshDSPGUIControlEnablement(btn);
                        }
                    }
                };

                tab->setExtraComponent(pbwp.release(), juce::TabBarButton::ExtraComponentPlacement::beforeText);
            }
        }
    }

    tabbedComponent.setTabColours();
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
        if (auto btn = dynamic_cast<PowerButtonWithParam*>(etbb->getExtraComponent()))
        {
			refreshDSPGUIControlEnablement(btn);
        }
    }
}

void JUCE_MultiFX_ProcessorAudioProcessorEditor::selectedTabChanged(int newCurrentTabIndex)
{
    if ( selectedTabAttachment )
    {
        rebuildInterface();
        tabbedComponent.setTabColours();
		selectedTabAttachment->setValueAsCompleteGesture(static_cast<float>(newCurrentTabIndex));
		
	}
}

void JUCE_MultiFX_ProcessorAudioProcessorEditor::refreshDSPGUIControlEnablement(PowerButtonWithParam* button)
{
    if (button != nullptr)
    {
        if ( auto bypass = button->getParam() )
        {
            dspGUI.toggleSliderEnablement(bypass->get() == false);
		}
    }
}