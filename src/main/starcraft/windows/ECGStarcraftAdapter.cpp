#include "ECGStarcraftAdapter.h"
#include <iostream>
#include <sstream>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "TransportBridge.h"

using namespace BWAPI;
using namespace Filter;

void ECGStarcraftAdapter::onStart() {
  int iResult;
  WSADATA wsaData;

  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    fprintf(stderr, "WSAStartup failed with error: %d\n", iResult);
    Broodwar->sendText("WSAStartup failed with error");
  }

  bridge = new TransportBridge();
  message = new rapidjson::Document();
  ntuple = new rapidjson::Document();
  response = new rapidjson::StringBuffer();
  responseWriter = new rapidjson::Writer<rapidjson::StringBuffer>(*response);

  // Enable the UserInput flag, which allows us to control the bot and type messages.
  Broodwar->enableFlag(Flag::UserInput);

  // Set the command optimization level so that common commands can be grouped
  // and reduce the bot's APM (Actions Per Minute).
  Broodwar->setCommandOptimizationLevel(2);

  // Retrieve you and your enemy's races. enemy() will just return the first enemy.
  if ( Broodwar->enemy() )
    Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;

}

void ECGStarcraftAdapter::onEnd(bool isWinner) {}

void ECGStarcraftAdapter::onFrame() {
  if (readMessage()) {
    if (strcmp((*ntuple)["action"].GetString(), "is_started") == 0) {
      setResponse("status", "success");
    }
    else if (strcmp((*ntuple)["action"].GetString(), "build") == 0) {
      build(getUnitType((*ntuple)["unit_type"].GetString()), (*ntuple)["quantity"].GetInt());
    }
    else if (strcmp((*ntuple)["action"].GetString(), "gather") == 0) {
      gather(getUnitType((*ntuple)["resource_type"].GetString()));
    }

    sendMessage();
  }

  // Called once every game frame

  // Display the game frame rate as text in the upper left area of the screen
  Broodwar->drawTextScreen(200, 0,  "FPS: %d", Broodwar->getFPS() );
  Broodwar->drawTextScreen(200, 20, "Average FPS: %f", Broodwar->getAverageFPS() );

  // Return if the game is a replay or is paused
  if ( Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self() )
    return;

  // Prevent spamming by only running our onFrame once every number of latency frames.
  // Latency frames are the number of frames before commands are processed.
  if ( Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0 )
    return;

  // Iterate through all the units that we own
  for (auto &u : Broodwar->self()->getUnits())
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
          if ( !u->gather( u->getClosestUnit( IsMineralField || IsRefinery )) )
          {
            // If the call fails, then print the last error message
            Broodwar << Broodwar->getLastError() << std::endl;
          }

        } // closure: has no powerup
      } // closure: if idle
    }
  } // closure: unit iterator
}

void ECGStarcraftAdapter::onSendText(std::string text) {
  // Send the text to the game if it is not being processed.
  Broodwar->sendText("%s", text.c_str());

  // Make sure to use %s and pass the text as a parameter,
  // otherwise you may run into problems when you use the %(percent) character!
}

void ECGStarcraftAdapter::onReceiveText(BWAPI::Player player, std::string text) {
  // Parse the received text
  Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
}

void ECGStarcraftAdapter::onPlayerLeft(BWAPI::Player player) {
  // Interact verbally with the other players in the game by
  // announcing that the other player has left.
  Broodwar->sendText("Goodbye %s!", player->getName().c_str());
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

void ECGStarcraftAdapter::onSaveGame(std::string gameName) {
  Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
}

void ECGStarcraftAdapter::onUnitComplete(BWAPI::Unit unit) {}

bool ECGStarcraftAdapter::readMessage() {
  if (bridge->data_available()) {
    bridge->recv_json(message);

    // Check if it's for me
    if (!strcmp((*message)[0].GetString(), "SHOUT") && !strcmp((*message)[2].GetString(), "StarCraft")) {

      // Printing the message
      if ((size_t)message->Capacity() > 3) {
        Broodwar->sendText((*message)[1].GetString());
        Broodwar->sendText((*message)[2].GetString());
        Broodwar->sendText((*message)[3].GetString());
      }

      ntuple->Parse((*message)[3].GetString());
      response->Clear(); // Clear the previous response
      responseWriter->StartArray();
      responseWriter->String("SHOUT");
      responseWriter->String("StarCraft");
      responseWriter->String((*message)[1].GetString());
      responseWriter->StartObject();
      return true;
    }
  }
  return false;
}

bool ECGStarcraftAdapter::sendMessage() {
  responseWriter->EndObject();
  responseWriter->EndArray();

  Broodwar << response->GetString() << std::endl;
  bridge->send_string(response->GetString());
  return true;
}

bool ECGStarcraftAdapter::setResponse(const std::string key, const std::string value) {
  responseWriter->Key(key.c_str());
  responseWriter->String(value.c_str());
  return true;
}

bool ECGStarcraftAdapter::setResponse(const std::string key, const int value) {
  responseWriter->Key(key.c_str());
  responseWriter->Int(value);
  return true;
}

bool ECGStarcraftAdapter::build(BWAPI::UnitType unitType, int count) {
  int minerals = Broodwar->self()->minerals();
  for (auto &u : Broodwar->self()->getUnits()) {
    if ( !u->exists() )
      continue;

    if (Broodwar->canMake(unitType, u) && Broodwar->self()->minerals() > unitType.mineralPrice()) {
      if (unitType.isBuilding()) {
        TilePosition targetBuildLocation = Broodwar->getBuildLocation(unitType, u->getTilePosition());
        if (u->build(unitType, targetBuildLocation)) {
          --count;
          break;
        }
      } else {
        if (Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed() >= unitType.supplyRequired() &&
            u->getTrainingQueue().size() < 5 && u->train(unitType)) {
          --count;
          break;
        }
      }
    }
  }

  if (count > 0) {
    setResponse("status", "failed");
    setResponse("remaining", count);
    return false;
  } else {
    setResponse("status", "success");
    return true;
  }
}

bool ECGStarcraftAdapter::gather(BWAPI::UnitType resourceType) {
  bool ordered = false;

  // Check first for idle workers and make them ALL gather the resource
  for (auto &u : Broodwar->self()->getUnits()) {
    if (!u->exists())
      continue;

    if (u->getType().isWorker()) {
      if (u->isIdle() && !u->isGatheringGas() && !u->isGatheringMinerals()) {

        if (resourceType == UnitTypes::Resource_Mineral_Field) {
          if (u->gather(u->getClosestUnit(IsMineralField)))
            ordered = true;
        } else if (resourceType == UnitTypes::Resource_Vespene_Geyser) {
          if (u->gather(u->getClosestUnit(IsRefinery)))
            ordered = true;
        }

      }
    }
  }

  if (ordered) {
    setResponse("status", "success");
    return true;
  }

  // Check for a worker gathering a different resource and make one gather the chosen resource
  for (auto &u : Broodwar->self()->getUnits()) {
    if ( !u->exists() )
      continue;

    if ( u->getType().isWorker() ) {
      if (resourceType == UnitTypes::Resource_Mineral_Field && u->isGatheringGas()) {
        if (u->gather(u->getClosestUnit(IsMineralField)))
          ordered = true;
      } else if (resourceType == UnitTypes::Resource_Vespene_Geyser && u->isGatheringMinerals()) {
        if (u->gather(u->getClosestUnit(IsRefinery)))
          ordered = true;
      }
    }

    if (ordered) {
      setResponse("status", "success");
      return true;
    }
  }

  setResponse("status", "failed");
  return false;
}

BWAPI::UnitType ECGStarcraftAdapter::getUnitType(const std::string unitName) {
  if (!unitName.compare("barracks")) {
    return UnitTypes::Terran_Barracks;
  } else if (!unitName.compare("refinery")) {
    return UnitTypes::Terran_Refinery;
  } else if (!unitName.compare("marine")) {
    return UnitTypes::Terran_Marine;
  } else if (!unitName.compare("scv")) {
    return UnitTypes::Terran_SCV;
  } else if (!unitName.compare("mineral")) {
    return UnitTypes::Resource_Mineral_Field;
  } else if (!unitName.compare("gas")) {
    return UnitTypes::Resource_Vespene_Geyser;
  }
}
