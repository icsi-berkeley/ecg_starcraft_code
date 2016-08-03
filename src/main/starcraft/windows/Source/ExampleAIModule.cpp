#include "ExampleAIModule.h"
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

void ExampleAIModule::onStart() {
  // Print the map name.
  // BWAPI returns std::string when retrieving a string, don't forget to add .c_str() when printing!
  Broodwar << "The map is " << Broodwar->mapName() << "!" << std::endl;

  // Enable the UserInput flag, which allows us to control the bot and type messages.
  Broodwar->enableFlag(Flag::UserInput);

  // Set the command optimization level so that common commands can be grouped
  // and reduce the bot's APM (Actions Per Minute).
  Broodwar->setCommandOptimizationLevel(2);

  // Retrieve you and your enemy's races. enemy() will just return the first enemy.
  if ( Broodwar->enemy() )
    Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;

}

void ExampleAIModule::onEnd(bool isWinner) {}

void ExampleAIModule::onFrame() {
  int iResult;
  WSADATA wsaData;

  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    fprintf(stderr, "WSAStartup failed with error: %d\n", iResult);
    Broodwar->sendText("WSAStartup failed with error");
  }

  // Create the transport bridge with default settings for
  // name and Bridge Server location.
  if (!tb) {
    tb = new TransportBridge();
    Broodwar->sendText("Created transport bridge");
  }
  // Place to store data from server.
  if (!doc) {
    doc = new rapidjson::Document();
    Broodwar->sendText("create rapidjson doc");
  }


  if (tb->data_available()) {
    tb->recv_json(doc);

    if ((size_t)doc->Capacity() > 3) {
      Broodwar->sendText((*doc)[3].GetString());
    }


    // Check if it's for me
    if (!strcmp((*doc)[0].GetString(), "SHOUT") &&
      !strcmp((*doc)[2].GetString(), "StarCraft")) {
      // Print message
      rapidjson::StringBuffer strbuf;
      rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
      (*doc)[3].Accept(writer);
      printf("Got message from %s: %s\n", (*doc)[1].GetString(), strbuf.GetString());

      rapidjson::Document ntuple = rapidjson::Document();
      ntuple.Parse((*doc)[3].GetString());
      // Send a reply to whoever sent the message.
      char charbuf[1024];
      if (!strcmp(ntuple["action"].GetString(), "is_started")) {
        _snprintf_s(charbuf, 1024, _TRUNCATE, "[\"SHOUT\", \"StarCraft\", \"%s\", \"{\\\"head\\\": \\\"%s\\\", \\\"status\\\": \\\"success\\\"}\"]", (*doc)[1].GetString(), ntuple["response_head"].GetString());
      }
      else if (!strcmp(ntuple["action"].GetString(), "build")) {
        int count = ntuple["count"].GetInt();
        Broodwar->sendText("count: %d", count);
        int minerals = Broodwar->self()->minerals();
        bool success = false;
        for (auto &u : Broodwar->self()->getUnits()) {
          if (!strcmp(ntuple["unit_type"].GetString(), "barracks")) {
            if (u->getType().isWorker()) {
              TilePosition targetBuildLocation = Broodwar->getBuildLocation(UnitTypes::Terran_Barracks, u->getTilePosition());
              if (targetBuildLocation) {
                if (minerals > UnitTypes::Terran_Barracks.mineralPrice() && Broodwar->canMake(UnitTypes::Terran_Barracks, u) && u->build(UnitTypes::Terran_Barracks, targetBuildLocation)) {
                  Broodwar->sendText("CAN BUILD BARRACKS");
                  minerals = minerals - UnitTypes::Terran_Barracks.mineralPrice();
                  count--;
                  if (count <= 0.0) {
                    _snprintf_s(charbuf, 1024, _TRUNCATE, "[\"SHOUT\", \"StarCraft\", \"%s\", \"{\\\"head\\\": \\\"%s\\\", \\\"status\\\": \\\"success\\\", \\\"remaining\\\": \\\"%d\\\"}\"]", (*doc)[1].GetString(), ntuple["response_head"].GetString(), count);
                    success = true;
                    break;
                  }
                }
              }
            }
          }
          else if (!strcmp(ntuple["unit_type"].GetString(), "grunt")) {
            if (Broodwar->canMake(UnitTypes::Terran_Marine, u) && u->train(UnitTypes::Terran_Marine)) {
              Broodwar->sendText("CAN BUILD MARINE");
              count--;
              if (count <= 0.0) {
                _snprintf_s(charbuf, 1024, _TRUNCATE, "[\"SHOUT\", \"StarCraft\", \"%s\", \"{\\\"head\\\": \\\"%s\\\", \\\"status\\\": \\\"success\\\", \\\"remaining\\\": \\\"%d\\\"}\"]", (*doc)[1].GetString(), ntuple["response_head"].GetString(), count);
                success = true;
                break;
              }
            }
          }
        }
        if (!success) {
          _snprintf_s(charbuf, 1024, _TRUNCATE, "[\"SHOUT\", \"StarCraft\", \"%s\", \"{\\\"head\\\": \\\"%s\\\", \\\"status\\\": \\\"failed\\\", \\\"remaining\\\": \\\"%d\\\"}\"]", (*doc)[1].GetString(), ntuple["response_head"].GetString(), count);

        }
      }
      tb->send_string(charbuf);
    }
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

void ExampleAIModule::onSendText(std::string text) {
  // Send the text to the game if it is not being processed.
  Broodwar->sendText("%s", text.c_str());

  // Make sure to use %s and pass the text as a parameter,
  // otherwise you may run into problems when you use the %(percent) character!
}

void ExampleAIModule::onReceiveText(BWAPI::Player player, std::string text) {
  // Parse the received text
  Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
}

void ExampleAIModule::onPlayerLeft(BWAPI::Player player) {
  // Interact verbally with the other players in the game by
  // announcing that the other player has left.
  Broodwar->sendText("Goodbye %s!", player->getName().c_str());
}

void ExampleAIModule::onNukeDetect(BWAPI::Position target) {}

void ExampleAIModule::onUnitDiscover(BWAPI::Unit unit) {}

void ExampleAIModule::onUnitEvade(BWAPI::Unit unit) {}

void ExampleAIModule::onUnitShow(BWAPI::Unit unit) {}

void ExampleAIModule::onUnitHide(BWAPI::Unit unit) {}

void ExampleAIModule::onUnitCreate(BWAPI::Unit unit) {}

void ExampleAIModule::onUnitDestroy(BWAPI::Unit unit) {}

void ExampleAIModule::onUnitMorph(BWAPI::Unit unit) {}

void ExampleAIModule::onUnitRenegade(BWAPI::Unit unit) {}

void ExampleAIModule::onSaveGame(std::string gameName) {
  Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
}

void ExampleAIModule::onUnitComplete(BWAPI::Unit unit) {}
