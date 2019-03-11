/*
  ==============================================================================

    DroneUI.cpp
    Created: 10 Mar 2019 8:50:00am
    Author:  bkupe

  ==============================================================================
*/

#include "DroneUI.h"
#include "MotionBlock/Model/ui/MotionBlockModelUI.h"

DroneUI::DroneUI(Drone * d) :
	BaseItemUI(d, Direction::NONE, Direction::NONE),
	isDraggingItemOver(false)
{
	item->addAsyncCoalescedDroneListener(this);
	updateDroneImage();
	updateBlockImage();
	setSize(100, 100);
}

DroneUI::~DroneUI()
{
	if(!inspectable.wasObjectDeleted()) item->removeAsyncDroneListener(this);
}

void DroneUI::paint(Graphics & g)
{
	BaseItemUI::paint(g);
	Rectangle<int> r = getMainBounds().withTop(headerHeight + headerGap);

	if (blockImage.getWidth() > 0) g.drawImage(blockImage, r.removeFromBottom(r.getHeight() / 3).toFloat(), RectanglePlacement::centred);
	if (droneImage.getWidth() > 0) g.drawImage(droneImage, r.toFloat().reduced(8), RectanglePlacement::centred);

	g.setColour(TEXT_COLOR);
	g.drawText(String(item->globalID->intValue()), getMainBounds().removeFromRight(20).removeFromBottom(20).toFloat(), Justification::centred);
}

void DroneUI::updateDroneImage()
{
	Drone::State s = item->state->getValueDataAsEnum<Drone::State>();
	String stateString = item->stateStrings[(int)s];

	int numBytes;
	const char * imgData = BinaryData::getNamedResource(("drone_" + stateString.toLowerCase() + "_png").getCharPointer(), numBytes);
	droneImage = ImageCache::getFromMemory(imgData, numBytes);
	repaint();
}

void DroneUI::updateBlockImage()
{
	if (item->currentBlock == nullptr)
	{
		blockImage = Image();
		return;
	}
	int numBytes;
	const char * imgData = BinaryData::getNamedResource( (StringUtil::toShortName(item->currentBlock->provider->getTypeString()) + "_png").getCharPointer(), numBytes);
	blockImage = ImageCache::getFromMemory(imgData, numBytes);
	repaint();
}

void DroneUI::controllableFeedbackUpdateInternal(Controllable * c)
{
	if (c == item->state) updateDroneImage();
	else if (c == item->globalID) repaint();
}

void DroneUI::newMessage(const Drone::DroneEvent & e)
{
	switch (e.type)
	{
	case Drone::DroneEvent::BLOCK_CHANGED:
		updateBlockImage();
		break;

	default:
		break;
	}
}


bool DroneUI::isInterestedInDragSource(const SourceDetails & source)
{
	return source.description.getProperty("type", "") == MotionBlockModelUI::dragAndDropID.toString();
}

void DroneUI::itemDragEnter(const SourceDetails & source)
{
	isDraggingItemOver = true;
	repaint();
}

void DroneUI::itemDragExit(const SourceDetails & source)
{
	isDraggingItemOver = false;
	repaint();
}

void DroneUI::itemDropped(const SourceDetails & source)
{
	MotionBlockModelUI * modelUI = dynamic_cast<MotionBlockModelUI *>(source.sourceComponent.get());

	if (modelUI != nullptr)
	{
		MotionBlockDataProvider * provider = modelUI->item;

		bool shift = KeyPress::isKeyCurrentlyDown(16);
		if (shift)
		{
			PopupMenu m;
			m.addItem(-1, "Default");
			m.addSeparator();
			int index = 1;
			for (auto &p : modelUI->item->presetManager.items) m.addItem(index++, p->niceName);
			int result = m.show();
			if (result >= 1) provider = modelUI->item->presetManager.items[result - 1];
		}

		item->activeProvider->setValueFromTarget(provider);
	}

	isDraggingItemOver = false;
	repaint();
}