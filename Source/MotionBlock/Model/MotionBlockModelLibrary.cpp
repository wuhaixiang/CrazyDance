/*
  ==============================================================================

	MotionBlockModelLibrary.cpp
	Created: 10 Apr 2018 10:57:18pm
	Author:  Ben

  ==============================================================================
*/

#include "MotionBlockModelLibrary.h"
#include "blocks/timeline/TimelineBlock.h"
#include "blocks/pattern/PatternBlock.h"

juce_ImplementSingleton(MotionBlockModelLibrary)


MotionBlockModelLibrary::MotionBlockModelLibrary() :
	ControllableContainer("Library"),
	genericBlocks("Generic"),
	scriptBlocks("Scripts", UserMotionBlockModelManager::SCRIPT),
	timelineBlocks("Timeline", UserMotionBlockModelManager::TIMELINE)
{
	//patterns
	positionBlock = new PositionPattern();
	lineBlock = new LinePattern();
	circleBlock = new CirclePattern();
	pingPongBlock = new PingPongPattern();
	multiPosBlock = new MultiPositionPattern();

	genericBlocks.addChildControllableContainer(positionBlock);
	genericBlocks.addChildControllableContainer(lineBlock);
	genericBlocks.addChildControllableContainer(circleBlock);
	genericBlocks.addChildControllableContainer(pingPongBlock);
	genericBlocks.addChildControllableContainer(multiPosBlock);
	addChildControllableContainer(&genericBlocks);

	addChildControllableContainer(&scriptBlocks);
	addChildControllableContainer(&timelineBlocks);

	iconSize = addIntParameter("Icon size", "Size of library icons", 70, 16, 120);
}

MotionBlockModelLibrary::~MotionBlockModelLibrary()
{

}


void MotionBlockModelLibrary::clear()
{
	Array<MotionBlockModel *> models = getAllModels(false);
	timelineBlocks.clear();
	scriptBlocks.clear();
	for (auto &m : models) m->clear();
}

var MotionBlockModelLibrary::getJSONData()
{
	var data = ControllableContainer::getJSONData();

	var gData = genericBlocks.getJSONData();
	if(gData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty("generic", gData);
	
	var uData = scriptBlocks.getJSONData();
	if (uData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty("scripts", uData);

	uData = timelineBlocks.getJSONData();
	if (uData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty("timelines", uData);


	return data;
}

void MotionBlockModelLibrary::loadJSONDataInternal(var data)
{
	genericBlocks.loadJSONData(data.getProperty("generic", var()));
	scriptBlocks.loadJSONData(data.getProperty("scripts", var()));
	timelineBlocks.loadJSONData(data.getProperty("timelines", var()));
}

Array<MotionBlockModel*> MotionBlockModelLibrary::getAllModels(bool includeUserModels)
{
	Array<MotionBlockModel*> result;

	for (auto &cc : genericBlocks.controllableContainers)
	{
		MotionBlockModel * m = dynamic_cast<MotionBlockModel *>(cc.get());
		if (m == nullptr) continue;
		result.add(m);
	}

	if (includeUserModels)
	{
		for (auto & m : scriptBlocks.items) result.add(m);
		for (auto & m : timelineBlocks.items) result.add(m);
	}

	return result;
}

MotionBlockModel * MotionBlockModelLibrary::getModelWithName(const String & modelName)
{
	Array<MotionBlockModel *> models = getAllModels();
	for (auto &m : models) if (m->shortName == modelName) return m;
	return nullptr;
}

MotionBlockDataProvider * MotionBlockModelLibrary::showProvidersAndGet()
{
	return showAllModelsAndGet(true);
}

MotionBlockDataProvider * MotionBlockModelLibrary::showAllModelsAndGet(bool includePresets)
{
	PopupMenu menu;
	Array<MotionBlockDataProvider *> mList = fillProvidersMenu(menu, includePresets);
	int result = menu.show();

	if (result > 0) return mList[result - 1];
	return nullptr;
}

Array<MotionBlockDataProvider *> MotionBlockModelLibrary::fillProvidersMenu(PopupMenu &menu, bool includePresets, int startIndex)
{
	PopupMenu genericMenu;
	PopupMenu liveFeedMenu;
	PopupMenu userMenu;

	Array<MotionBlockDataProvider *> mList;

	int index = startIndex;
	for (auto &cc : MotionBlockModelLibrary::getInstance()->genericBlocks.controllableContainers)
	{
		MotionBlockModel * m = dynamic_cast<MotionBlockModel *>(cc.get());
		if (m == nullptr) continue;

		if (includePresets)
		{
			PopupMenu modelMenu;

			modelMenu.addItem(index, "Default");
			mList.add(m);
			index++;

			for (auto &mp : m->presetManager.items)
			{
				modelMenu.addItem(index, mp->niceName);
				mList.add(mp);
				index++;
			}

			genericMenu.addSubMenu(m->niceName, modelMenu);
		} else
		{
			genericMenu.addItem(index, m->niceName);
			mList.add(m);
			index++;
		}
	}

	
	menu.addSubMenu(MotionBlockModelLibrary::getInstance()->genericBlocks.niceName, genericMenu);
	
	PopupMenu scriptsMenu;
	Array<MotionBlockDataProvider *> pa = fillUserMotionBlockManagerMenu(&MotionBlockModelLibrary::getInstance()->scriptBlocks, scriptsMenu, includePresets, index);
	index += pa.size();
	mList.addArray(pa);
	menu.addSubMenu(MotionBlockModelLibrary::getInstance()->scriptBlocks.niceName, scriptsMenu);

	PopupMenu timelinesMenu;
	pa = fillUserMotionBlockManagerMenu(&MotionBlockModelLibrary::getInstance()->timelineBlocks, timelinesMenu, includePresets, index);
	index += pa.size();
	mList.addArray(pa);
	menu.addSubMenu(MotionBlockModelLibrary::getInstance()->timelineBlocks.niceName, timelinesMenu);

	return mList;
}

Array<MotionBlockDataProvider*> MotionBlockModelLibrary::fillUserMotionBlockManagerMenu(UserMotionBlockModelManager * manager, PopupMenu & menu, bool includePresets, int startIndex)
{
	Array<MotionBlockDataProvider*> mList;

	int index = startIndex;
	for (auto & m : manager->items)
	{
		if (includePresets)
		{
			PopupMenu modelMenu;

			modelMenu.addItem(index, "Default");
			mList.add(m);
			index++;
			for (auto &mp : m->presetManager.items)
			{
				modelMenu.addItem(index, mp->niceName);
				mList.add(mp);
				index++;
			}

			menu.addSubMenu(m->niceName, modelMenu);
		} else
		{
			menu.addItem(index, m->niceName);
			mList.add(m);
			index++;
		}
	}

	return mList;
}
