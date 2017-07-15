#include "ECGStarcraftAdapter.h"
#include <iostream>
#include <sstream>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include "BWTA.h"
#include "ECGUtil.h"
#include "MessageManager.h"
#include "Message.h"
#include "ECGStarcraftManager.h"
#include "InformationManager.h"
#include "MapGrid.h"
#include "WorkerManager.h"
#include "ProductionManager.h"
#include "BuildingManager.h"
#include "NameManager.h"

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

  // Get BWTA ready for the InformationManager
  BWTA::readMap();
	BWTA::analyze();

}

void ECGStarcraftAdapter::onEnd(bool isWinner) {}

void ECGStarcraftAdapter::onFrame()
{
  // Called once every game frame

  // Display the game frame rate as text in the upper left area of the screen
  BWAPI::Broodwar->drawTextScreen(10, 10,  "FPS: %d", BWAPI::Broodwar->getFPS() );
  BWAPI::Broodwar->drawTextScreen(10, 20, "Average FPS: %f", BWAPI::Broodwar->getAverageFPS() );
  BWAPI::Broodwar->drawTextScreen(10, 30, "APM: %f", BWAPI::Broodwar->getAPM() );

  NameManager::Instance().draw();

  // Return if the game is a replay or is paused
  if ( BWAPI::Broodwar->isReplay() || BWAPI::Broodwar->isPaused() || !BWAPI::Broodwar->self() )
    return;

  // Prevent spamming by only running our onFrame once every number of latency frames.
  // Latency frames are the number of frames before commands are processed.
  if ( BWAPI::Broodwar->getFrameCount() % BWAPI::Broodwar->getLatencyFrames() != 0 )
    return;

  // MessageManager::Instance().sendRequest();
  while (MessageManager::Instance().readIncoming())
  {
  	Message* currentMessage = MessageManager::Instance().current();
  	if (currentMessage->isStarted())
    {
      MessageManager::Instance().sendStarted();
    }
  	else if (currentMessage->isConditional())
    {
      return; // TODO: Handle conditionals
    }
  	else if (currentMessage->isSequential())
    {
      return; // TODO: Handle sequentials
    }
  	else if (strcmp(currentMessage->readType(), "build") == 0)
    {
      BWAPI::UnitType unitType = currentMessage->readUnitType();
      int quantity = currentMessage->readQuantity();
      ECGStarcraftManager::Instance().build(UnitDescriptor(), unitType, quantity);
    }
  	else if (strcmp(currentMessage->readType(), "gather") == 0)
    {
      // TODO: It makes more sense to just pass the message
      ECGStarcraftManager::Instance().gather(currentMessage->readCommandedUnit(), currentMessage->readResourceType());
    }

  }

  UAlbertaBot::InformationManager::Instance().update();
  UAlbertaBot::MapGrid::Instance().update();
  UAlbertaBot::WorkerManager::Instance().update();
  UAlbertaBot::ProductionManager::Instance().update();
  UAlbertaBot::BuildingManager::Instance().update();

}

void ECGStarcraftAdapter::onSendText(std::string text)
{
  // Send the text to the game if it is not being processed.
  BWAPI::Broodwar->sendText("%s", text.c_str());

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
  UAlbertaBot::InformationManager::Instance().onUnitShow(unit);
	UAlbertaBot::WorkerManager::Instance().onUnitShow(unit);
  NameManager::Instance().onUnitShow(unit);
}

void ECGStarcraftAdapter::onUnitHide(BWAPI::Unit unit)
{
  UAlbertaBot::InformationManager::Instance().onUnitHide(unit);
}

void ECGStarcraftAdapter::onUnitCreate(BWAPI::Unit unit)
{
  NameManager::Instance().onUnitCreate(unit);
  UAlbertaBot::InformationManager::Instance().onUnitCreate(unit);
}

void ECGStarcraftAdapter::onUnitDestroy(BWAPI::Unit unit)
{
  // TODO: Remove the unit label
  UAlbertaBot::ProductionManager::Instance().onUnitDestroy(unit);
	UAlbertaBot::WorkerManager::Instance().onUnitDestroy(unit);
	UAlbertaBot::InformationManager::Instance().onUnitDestroy(unit);
}

void ECGStarcraftAdapter::onUnitMorph(BWAPI::Unit unit) {}

void ECGStarcraftAdapter::onUnitRenegade(BWAPI::Unit unit) {}

void ECGStarcraftAdapter::onSaveGame(std::string gameName) {}

void ECGStarcraftAdapter::onUnitComplete(BWAPI::Unit unit)
{
  UAlbertaBot::InformationManager::Instance().onUnitComplete(unit);
}
