/*
  ==============================================================================

    CDEngine.cpp
    Created: 10 Mar 2019 9:05:52am
    Author:  bkupe

  ==============================================================================
*/

#include "CDEngine.h"

#include "CDEngine.h"
#include "MotionBlock/model/MotionBlockModelLibrary.h"
#include "Drone/DroneManager.h"
#include "Audio/AudioManager.h"
#include "Timeline/layers/Block/MotionBlockLayer.h"
#include "Timeline/layers/Color/ColorLayer.h"
#include "Timeline/layers/Action/ActionLayer.h"
#include "Drone/Cluster/DroneClusterGroupManager.h"

CDEngine::CDEngine() :
	Engine("CrazyFance", ".dance"),
	ioCC("IO")
{
	Engine::mainEngine = this;

	addChildControllableContainer(MotionBlockModelLibrary::getInstance());
	addChildControllableContainer(DroneManager::getInstance());
	addChildControllableContainer(DroneClusterGroupManager::getInstance());

	remoteHost = ioCC.addStringParameter("Remote Host", "Global remote host to send OSC to", "127.0.0.1");
	remotePort = ioCC.addIntParameter("Remote port", "Remote port to send OSC to", 43001, 1024, 65535);
	globalSender.connect("0.0.0.0", 1024);

	ProjectSettings::getInstance()->addChildControllableContainer(&ioCC);
	ProjectSettings::getInstance()->addChildControllableContainer(AudioManager::getInstance());

	//Timeline
	SequenceLayerFactory::getInstance()->layerDefs.add(SequenceLayerDefinition::createDef("Motion", &MotionBlockLayer::create));
	SequenceLayerFactory::getInstance()->layerDefs.add(SequenceLayerDefinition::createDef(ColorLayer::getTypeStringStatic(), &ColorLayer::create));
	SequenceLayerFactory::getInstance()->layerDefs.add(SequenceLayerDefinition::createDef("Actions", &ActionLayer::create));
	SequenceLayerFactory::getInstance()->layerDefs.add(SequenceLayerDefinition::createDef("Audio", &AudioLayer::create));

	//Communication
	OSCRemoteControl::getInstance()->addRemoteControlListener(this);

}

CDEngine::~CDEngine()
{
	DroneManager::getInstance()->clear();

	DroneManager::deleteInstance();
	DroneClusterGroupManager::deleteInstance();

	MotionBlockModelLibrary::deleteInstance();
	AudioManager::deleteInstance();
	SequenceLayerFactory::deleteInstance();
}

void CDEngine::clearInternal()
{
	DroneManager::getInstance()->clear();
	DroneClusterGroupManager::getInstance()->clear();
	MotionBlockModelLibrary::getInstance()->clear();
}


void CDEngine::processMessage(const OSCMessage & m)
{
	StringArray aList;
	aList.addTokens(m.getAddressPattern().toString(), "/", "\"");

	if (aList.size() < 2) return;

	if (aList[1] == "model")
	{
		String modelName = OSCHelpers::getStringArg(m[0]);
		MotionBlockModel * lm = MotionBlockModelLibrary::getInstance()->getModelWithName(modelName);

		if (lm != nullptr)
		{
			if (aList[2] == "assign")
			{
				if (m.size() >= 2)
				{
					int id = OSCHelpers::getIntArg(m[1]);

					MotionBlockModelPreset * mp = nullptr;
					if (m.size() >= 3)
					{
						String presetName = OSCHelpers::getStringArg(m[2]);
						mp = lm->presetManager.getItemWithName(presetName);
					}

					MotionBlockDataProvider * providerToAssign = mp != nullptr ? mp : (MotionBlockDataProvider *)lm;
					if (id == -1)
					{
						for (auto & p : DroneManager::getInstance()->items)  p->activeProvider->setValueFromTarget(providerToAssign);
					}
					else
					{
						Drone * p = DroneManager::getInstance()->getDroneWithId(id);
						if (p != nullptr) p->activeProvider->setValueFromTarget(providerToAssign);
					}

				}


			}
		}

	}
	else if (aList[1] == "Drone")
	{
		int id = OSCHelpers::getIntArg(m[0]);
		//Drone * p = DroneManager::getInstance()->getDroneWithId(id);

		if (aList[2] == "enable")
		{
			bool active = OSCHelpers::getIntArg(m[1]) > 0;

			if (id == -1)
			{
				for (auto & p : DroneManager::getInstance()->items)  p->enabled->setValue(active);
			}
			else
			{
				Drone * p = DroneManager::getInstance()->getDroneWithId(id);
				if (p != nullptr) p->enabled->setValue(active);
			}

		}
	}
}


var CDEngine::getJSONData()
{
	var data = Engine::getJSONData();

	var mData = MotionBlockModelLibrary::getInstance()->getJSONData();
	if (!mData.isVoid() && mData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty("models", mData);


	var DroneData = DroneManager::getInstance()->getJSONData();
	if (!DroneData.isVoid() && DroneData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty("Drones", DroneData);

	var clusterData = DroneClusterGroupManager::getInstance()->getJSONData();
	if (!clusterData.isVoid() && clusterData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty("clusters", clusterData);

	return data;
}

void CDEngine::loadJSONDataInternalEngine(var data, ProgressTask * loadingTask)
{
	//ProgressTask * projectTask = loadingTask->addTask("Project"
	ProgressTask * modelsTask = loadingTask->addTask("Models");
	ProgressTask * DroneTask = loadingTask->addTask("Drones");
	ProgressTask * clusterTask = loadingTask->addTask("Clusters");


	clusterTask->start();
	DroneClusterGroupManager::getInstance()->loadJSONData(data.getProperty("clusters", var()));
	clusterTask->setProgress(1);
	clusterTask->end();

	modelsTask->start();
	MotionBlockModelLibrary::getInstance()->loadJSONData(data.getProperty("models", var()));
	modelsTask->setProgress(1);
	modelsTask->end();


	DroneTask->start();
	DroneManager::getInstance()->loadJSONData(data.getProperty("Drones", var()));
	DroneTask->setProgress(1);
	DroneTask->end();
}

