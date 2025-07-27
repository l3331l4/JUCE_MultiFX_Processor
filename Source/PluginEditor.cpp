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

juce::Array<juce::TabBarButton*> ExtendedTabbedButtonBar::getTabs()
{
    auto numTabs = getNumTabs();
    auto tabs = juce::Array<juce::TabBarButton*>();
    tabs.resize(numTabs);
    for (int i = 0; i < numTabs; ++i)
    {
        tabs.getReference(i) = getTabButton(i);
    }
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

        if (previousTab == nullptr && nextTab != nullptr)
        {
            if (tabButtonBeingDragged->getX() > nextTab->getBounds().getCentreX())
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
            if (tabButtonBeingDragged->getX() > nextTab->getBounds().getCentreX())
            {
                moveTab(idx, nextTabIndex);
            }
            else if (tabButtonBeingDragged->getX() < previousTab->getBounds().getCentreX())
            {
                moveTab(idx, previousTabIndex);
            }
        }
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
				tabbedComponent.addTab(getDSPOptionName(option), juce::Colours::red, -1);
            }
			DBG(juce::Base64::toBase64(dspOrder.data(), dspOrder.size()));

			audioProcessor.dspOrderFifo.push(dspOrder);

		};


	addAndMakeVisible(dspOrderButton);
	addAndMakeVisible(tabbedComponent);
	tabbedComponent.addListener(this);
    setSize (400, 300);
}

JUCE_MultiFX_ProcessorAudioProcessorEditor::~JUCE_MultiFX_ProcessorAudioProcessorEditor()
{
	tabbedComponent.removeListener(this);
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

void JUCE_MultiFX_ProcessorAudioProcessorEditor::tabOrderChanged(JUCE_MultiFX_ProcessorAudioProcessor::DSP_Order newOrder)
{
	audioProcessor.dspOrderFifo.push(newOrder);
}
