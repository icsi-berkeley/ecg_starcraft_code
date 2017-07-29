#include "ECGStarcraftAdapter.h"
#include <iostream>
#include <sstream>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include "ECGUtil.h"
#include "MessageManager.h"
#include "Message.h"
#include "ECGStarcraftManager.h"
#include "NameManager.h"
#include "EventManager.h"

#include "Global.h"

using namespace ECGBot;

void ECGStarcraftAdapter::onStart()
{
  int iResult;
  WSADATA wsaData;

  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0)
  {
    fprintf(stderr, "WSAStartup failed with error: %d\n", iResult);
    BWAPI::Broodwar->sendText("WSAStartup failed with error");
  }

  // Enable the UserInput flag, which allows us to control the bot and type messages.
  BWAPI::Broodwar->enableFlag(BWAPI::Flag::UserInput);

  // Set the command optimization level so that common commands can be grouped
  // and reduce the bot's APM (Actions Per Minute).
  BWAPI::Broodwar->setCommandOptimizationLevel(2);

  UAlbertaBot::Global::SetModule(this);

  _unitInfoManager.onStart();
  _mapTools.onStart();
  _baseLocationManager.onStart();
  _productionManager.onStart();

}

void ECGStarcraftAdapter::onEnd(bool isWinner) {}

void ECGStarcraftAdapter::onFrame()
{
  // Called once every game frame

  // Display the game frame rate as text in the upper left area of the screen
  BWAPI::Broodwar->drawTextScreen(10, 10,  "FPS: %d", BWAPI::Broodwar->getFPS() );
  BWAPI::Broodwar->drawTextScreen(10, 20, "Average FPS: %f", BWAPI::Broodwar->getAverageFPS() );
  BWAPI::Broodwar->drawTextScreen(10, 30, "APM: %d", BWAPI::Broodwar->getAPM() );

  NameManager::Instance().draw();
  _productionManager.drawProductionInformation(200, 10); // TODO:albertanewversionfix

  // Return if the game is a replay or is paused
  if ( BWAPI::Broodwar->isReplay() || BWAPI::Broodwar->isPaused() || !BWAPI::Broodwar->self() )
    return;

  // Prevent spamming by only running our onFrame once every number of latency frames.
  // Latency frames are the number of frames before commands are processed.
  if ( BWAPI::Broodwar->getFrameCount() % BWAPI::Broodwar->getLatencyFrames() != 0 )
    return;

  while (MessageManager::Instance().readIncoming())
  {
    Message* currentMessage = MessageManager::Instance().current();
    if (currentMessage->isStarted())
      MessageManager::Instance().sendStarted();
    else if (currentMessage->isConditional())
      EventManager::Instance().registerEvent(currentMessage);
    else
      ECGStarcraftManager::Instance().evaluateAction(currentMessage);
  }

  _mapTools.update();
  _strategyManager.update();
  _unitInfoManager.update();
  _workerManager.update();
  _baseLocationManager.update();
	_productionManager.update();

  if ( BWAPI::Broodwar->getFrameCount() % (BWAPI::Broodwar->getLatencyFrames() * 8) == 0 )
    EventManager::Instance().update();

  // One of my ugliest hacks ever in order to getBuildUnit which for some reason is not available onUnitCreate
  for (auto unit : BWAPI::Broodwar->self()->getUnits())
  {
    if (unit->isTraining() || unit->isConstructing())
    {
      if (unit->getBuildUnit())
        NameManager::Instance().onUnitReadyFrame(unit->getID(), unit->getBuildUnit()->getID());
    }
  }

}

void ECGStarcraftAdapter::onSendText(std::string text)
{
  // Send the text to the game if it is not being processed.
  BWAPI::Broodwar->sendText("%s", text.c_str());
  MessageManager::Instance().sendCommandText(text);

  // Make sure to use %s and pass the text as a parameter,
  // otherwise you may run into problems when you use the %(percent) character!
}

void ECGStarcraftAdapter::onReceiveText(BWAPI::Player player, std::string text)
{
  // Parse the received text
  BWAPI::Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
}

void ECGStarcraftAdapter::onPlayerLeft(BWAPI::Player player) {}

void ECGStarcraftAdapter::onNukeDetect(BWAPI::Position target) {}

void ECGStarcraftAdapter::onUnitDiscover(BWAPI::Unit unit) {}

void ECGStarcraftAdapter::onUnitEvade(BWAPI::Unit unit) {}

void ECGStarcraftAdapter::onUnitShow(BWAPI::Unit unit)
{
  _unitInfoManager.onUnitShow(unit);
	_workerManager.onUnitShow(unit);
  NameManager::Instance().onUnitShow(unit);
}

void ECGStarcraftAdapter::onUnitHide(BWAPI::Unit unit)
{
  _unitInfoManager.onUnitHide(unit);
}

void ECGStarcraftAdapter::onUnitCreate(BWAPI::Unit unit)
{
  NameManager::Instance().onUnitCreate(unit);
  _unitInfoManager.onUnitCreate(unit);
}

void ECGStarcraftAdapter::onUnitDestroy(BWAPI::Unit unit)
{
  // TODO: Remove the unit label
  _workerManager.onUnitDestroy(unit);
	_unitInfoManager.onUnitDestroy(unit);
  _productionManager.onUnitDestroy(unit);

}

void ECGStarcraftAdapter::onUnitMorph(BWAPI::Unit unit)
{
  _unitInfoManager.onUnitMorph(unit);
	_workerManager.onUnitMorph(unit);
}

void ECGStarcraftAdapter::onUnitRenegade(BWAPI::Unit unit)
{
  _unitInfoManager.onUnitRenegade(unit);
}

void ECGStarcraftAdapter::onSaveGame(std::string gameName) {}

void ECGStarcraftAdapter::onUnitComplete(BWAPI::Unit unit)
{
  _unitInfoManager.onUnitComplete(unit);
  EventManager::Instance().onUnitComplete(unit);
}

UAlbertaBot::WorkerManager & ECGStarcraftAdapter::Workers()
{
    return _workerManager;
}

UAlbertaBot::ProductionManager & ECGStarcraftAdapter::Production()
{
    return _productionManager;
}

const UAlbertaBot::UnitInfoManager & ECGStarcraftAdapter::UnitInfo() const
{
    return _unitInfoManager;
}

const UAlbertaBot::StrategyManager & ECGStarcraftAdapter::Strategy() const
{
    return _strategyManager;
}

const UAlbertaBot::BaseLocationManager & ECGStarcraftAdapter::Bases() const
{
    return _baseLocationManager;
}

const UAlbertaBot::MapTools & ECGStarcraftAdapter::Map() const
{
    return _mapTools;
}
