#include "ECGStarcraftAdapter.h"
#include <iostream>
#include <sstream>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include "BWTA.h"
#include "ECGUtil.h"
#include "MessageManager.h"

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
  BWAPI::Broodwar->drawTextScreen(200, 0,  "FPS: %d", BWAPI::Broodwar->getFPS() );
  BWAPI::Broodwar->drawTextScreen(200, 20, "Average FPS: %f", BWAPI::Broodwar->getAverageFPS() );

  // Labels all units as Alpha
  for (auto &uname : BWAPI::Broodwar->self()->getUnits())
    BWAPI::Broodwar->drawTextMap(uname->getLeft(), uname->getTop(), "Alpha" );

  // Return if the game is a replay or is paused
  if ( BWAPI::Broodwar->isReplay() || BWAPI::Broodwar->isPaused() || !BWAPI::Broodwar->self() )
    return;

  // Prevent spamming by only running our onFrame once every number of latency frames.
  // Latency frames are the number of frames before commands are processed.
  if ( BWAPI::Broodwar->getFrameCount() % BWAPI::Broodwar->getLatencyFrames() != 0 )
    return;

  MessageManager::Instance().sendRequest();
  if (!MessageManager::Instance().readResponse())
  {
    return;
    // if (strcmp((*response)["action"].GetString(), "is_started") == 0)
    // {
    //   // setRequest("status", "success");
    // }
    // else if (strcmp((*response)["action"].GetString(), "build") == 0)
    // {
    //   build(ECGUtil::getUnitType((*response)["unit_type"].GetString()), (*response)["quantity"].GetInt());
    // }
    // else if (strcmp((*response)["action"].GetString(), "gather") == 0)
    // {
    //   gather(ECGUtil::getUnitType((*response)["resource_type"].GetString()));
    // }

  }

  // Iterate through all the units that we own
  for (auto &u : BWAPI::Broodwar->self()->getUnits())
  {
    // Ignore the unit if it no longer exists
    // Make sure to include this block when handling any Unit pointer!
    if ( !u->exists() )
      continue;

    // Ignore the unit if it has one of the following status ailments
    if ( u->isLockedDown() || u->isMaelstrommed() || u->isStasised() )
      continue;

    // Ignore the unit if it is in one of the following states
    if ( u->isLoaded() || !u->isPowered() || u->isStuck() )
      continue;

    // Ignore the unit if it is incomplete or busy constructing
    if ( !u->isCompleted() || u->isConstructing() )
      continue;

    // If the unit is a worker unit
    if ( u->getType().isWorker() )
    {
      // if our worker is idle
      if ( u->isIdle() )
      {
        continue; // TODO: Added to disable the automatic assignment of idle workers
        // Order workers carrying a resource to return them to the center,
        // otherwise find a mineral patch to harvest.
        if ( u->isCarryingGas() || u->isCarryingMinerals() )
        {
          u->returnCargo();
        }
        else if ( !u->getPowerUp() )  // The worker cannot harvest anything if it
        {                             // is carrying a powerup such as a flag
          // Harvest from the nearest mineral patch or gas refinery
          if ( !u->gather( u->getClosestUnit( BWAPI::Filter::IsMineralField || BWAPI::Filter::IsRefinery )) )
          {
            // If the call fails, then print the last error message
            BWAPI::Broodwar << BWAPI::Broodwar->getLastError() << std::endl;
          }

        } // closure: has no powerup
      } // closure: if idle
    }
  } // closure: unit iterator
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

void ECGStarcraftAdapter::onPlayerLeft(BWAPI::Player player)
{
  // Interact verbally with the other players in the game by
  // announcing that the other player has left.
  BWAPI::Broodwar->sendText("Goodbye %s!", player->getName().c_str());
}

void ECGStarcraftAdapter::onNukeDetect(BWAPI::Position target) {}

void ECGStarcraftAdapter::onUnitDiscover(BWAPI::Unit unit) {}

void ECGStarcraftAdapter::onUnitEvade(BWAPI::Unit unit) {}

void ECGStarcraftAdapter::onUnitShow(BWAPI::Unit unit) {}

void ECGStarcraftAdapter::onUnitHide(BWAPI::Unit unit) {}

void ECGStarcraftAdapter::onUnitCreate(BWAPI::Unit unit) {}

void ECGStarcraftAdapter::onUnitDestroy(BWAPI::Unit unit) {}

void ECGStarcraftAdapter::onUnitMorph(BWAPI::Unit unit) {}

void ECGStarcraftAdapter::onUnitRenegade(BWAPI::Unit unit) {}

void ECGStarcraftAdapter::onSaveGame(std::string gameName)
{
  BWAPI::Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
}

void ECGStarcraftAdapter::onUnitComplete(BWAPI::Unit unit) {}

bool ECGStarcraftAdapter::build(BWAPI::UnitType unitType, int count)
{
  int minerals = BWAPI::Broodwar->self()->minerals();
  for (auto &u : BWAPI::Broodwar->self()->getUnits())
  {
    if ( !u->exists() )
      continue;

    if (BWAPI::Broodwar->canMake(unitType, u) && BWAPI::Broodwar->self()->minerals() > unitType.mineralPrice())
    {
      if (unitType.isBuilding())
      {
        BWAPI::TilePosition targetBuildLocation = BWAPI::Broodwar->getBuildLocation(unitType, u->getTilePosition());
        if (u->build(unitType, targetBuildLocation))
        {
          --count;
          break;
        }
      }
      else
      {
        if (BWAPI::Broodwar->self()->supplyTotal() - BWAPI::Broodwar->self()->supplyUsed() >= unitType.supplyRequired() &&
            u->getTrainingQueue().size() < 5 && u->train(unitType))
        {
          --count;
          break;
        }
      }
    }
  }

  if (count > 0)
  {
    // setRequest("status", "failed");
    // setRequest("remaining", count);
    return false;
  }
  else
  {
    // setRequest("status", "success");
    return true;
  }
}

bool ECGStarcraftAdapter::gather(BWAPI::UnitType resourceType)
{
  bool ordered = false;

  // Check first for idle workers and make them ALL gather the resource
  for (auto &u : BWAPI::Broodwar->self()->getUnits())
  {
    if (!u->exists())
      continue;

    if (u->getType().isWorker())
    {
      if (u->isIdle() && !u->isGatheringGas() && !u->isGatheringMinerals())
      {

        if (resourceType == BWAPI::UnitTypes::Resource_Mineral_Field && u->gather(u->getClosestUnit(BWAPI::Filter::IsMineralField)))
        {
          ordered = true;
        }
        else if (resourceType == BWAPI::UnitTypes::Resource_Vespene_Geyser && u->gather(u->getClosestUnit(BWAPI::Filter::IsRefinery)))
        {
          ordered = true;
        }

      }
    }
  }

  if (ordered)
  {
    // setRequest("status", "success");
    return true;
  }

  // Check for a worker gathering a different resource and make one gather the chosen resource
  for (auto &u : BWAPI::Broodwar->self()->getUnits()) {
    if ( !u->exists() )
      continue;

    if ( u->getType().isWorker() )
    {
      if (resourceType == BWAPI::UnitTypes::Resource_Mineral_Field && u->isGatheringGas())
      {
        if (u->gather(u->getClosestUnit(BWAPI::Filter::IsMineralField)))
          ordered = true;
      }
      else if (resourceType == BWAPI::UnitTypes::Resource_Vespene_Geyser && u->isGatheringMinerals())
      {
        if (u->gather(u->getClosestUnit(BWAPI::Filter::IsRefinery)))
          ordered = true;
      }
    }

    if (ordered)
    {
      // setRequest("status", "success");
      return true;
    }
  }

  // setRequest("status", "failed");
  return false;
}
