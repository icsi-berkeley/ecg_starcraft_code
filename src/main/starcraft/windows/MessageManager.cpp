#include "MessageManager.h"
#include "InformationManager.h"
#include "UnitData.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "TransportBridge.h"
#include "ECGUtil.h"
#include "BWTA.h"

using namespace ECGBot;

MessageManager::MessageManager()
{
	initializeTransport();
}

MessageManager & MessageManager::Instance()
{
	static MessageManager instance;
	return instance;
}

void MessageManager::initializeTransport()
{
  bridge = new TransportBridge();
  message = new rapidjson::Document();
  response = new rapidjson::Document();
  request = new rapidjson::StringBuffer();
  requestWriter = new rapidjson::Writer<rapidjson::StringBuffer>(*request);
}


void MessageManager::sendRequest()
{
  request->Clear(); // Clear the previous request

  requestWriter->StartArray();
  requestWriter->String("SHOUT"); // Type of message
  requestWriter->String("StarCraft"); // Source of message
  requestWriter->String("FED1_ProblemSolver"); // Destination of message
  requestWriter->StartObject();

  // Populate the object
  UAlbertaBot::InformationManager::Instance().update();
  serializeInformation();

  requestWriter->EndObject();
  requestWriter->EndArray();

  BWAPI::Broodwar << request->GetString() << std::endl;
  bridge->send_string(request->GetString());
}

bool MessageManager::readResponse()
{
  if (bridge->data_available((long) 0.05))
  {
    bridge->recv_json(message);

    // Check if it's for me
    if (!strcmp((*message)[0].GetString(), "SHOUT") && !strcmp((*message)[2].GetString(), "StarCraft"))
    {

      // Printing the message
      if ((size_t)message->Capacity() > 3)
      {
        BWAPI::Broodwar->sendText((*message)[1].GetString()); // Source
        BWAPI::Broodwar->sendText((*message)[2].GetString()); // Destination
        BWAPI::Broodwar->sendText((*message)[3].GetString()); // Message
      }

      response->Parse((*message)[3].GetString()); // Unpack response
      return true;
    }
  }
  return false;
}

void MessageManager::serializeInformation()
{
	BWAPI::Player self = BWAPI::Broodwar->self();
	BWAPI::Player enemy = BWAPI::Broodwar->enemy();

	requestWriter->Key("self");
	serializePlayer(self);

	requestWriter->Key("enemy");
	serializePlayer(enemy);
}

void MessageManager::serializePlayer(BWAPI::Player player)
{
	requestWriter->StartObject();
	requestWriter->Key("units");
	serializeUnits(player);

	requestWriter->Key("base");
	serializeBaseLocation(player);

	requestWriter->Key("resources");
	serializeResources(player);
	requestWriter->EndObject();
}

void MessageManager::serializeUnits(BWAPI::Player player)
{
	requestWriter->StartArray();
	for (const auto & unitIter : UAlbertaBot::InformationManager::Instance().getUnitInfo(player))
	{
		requestWriter->StartObject();
		const UAlbertaBot::UnitInfo & unitInfo = unitIter.second;

		requestWriter->Key("id");
		requestWriter->Int(unitInfo.unitID);

		requestWriter->Key("type");
		requestWriter->String(ECGUtil::getUnitName(unitInfo.type).c_str());

		requestWriter->Key("completed");
		requestWriter->Bool(unitInfo.completed);

		requestWriter->Key("health");
		requestWriter->Int(unitInfo.lastHealth);

		requestWriter->Key("shields");
		requestWriter->Int(unitInfo.lastShields);

		requestWriter->Key("positionX");
		requestWriter->Int(unitInfo.lastPosition.x);

		requestWriter->Key("positionY");
		requestWriter->Int(unitInfo.lastPosition.y);

		requestWriter->EndObject();
	}
	requestWriter->EndArray();
}

void MessageManager::serializeBaseLocation(BWAPI::Player player)
{
	requestWriter->StartObject();
	BWTA::BaseLocation* base = UAlbertaBot::InformationManager::Instance().getMainBaseLocation(player);
	requestWriter->Key("positionX");
	requestWriter->Int(base->getPosition().x);

	requestWriter->Key("positionY");
	requestWriter->Int(base->getPosition().y);

	requestWriter->Key("numAvailableMinerals");
	requestWriter->Int(base->minerals());

	requestWriter->Key("numAvilableGas");
	requestWriter->Int(base->gas());

	requestWriter->Key("availableMinerals");
	requestWriter->StartArray();
	for (BWAPI::Unitset::iterator iter = base->getMinerals().begin(); iter != base->getMinerals().end(); iter++)
	{
		requestWriter->StartObject();
		requestWriter->Key("id");
		requestWriter->Int((*iter)->getID());

		requestWriter->Key("positionX");
		requestWriter->Int((*iter)->getInitialPosition().x);

		requestWriter->Key("positionY");
		requestWriter->Int((*iter)->getInitialPosition().y);

		requestWriter->Key("remaining");
		requestWriter->Int((*iter)->getResources());
		requestWriter->EndObject();
	}
	requestWriter->EndArray();

	requestWriter->Key("availableGas");
	requestWriter->StartArray();
	for (BWAPI::Unitset::iterator iter = base->getGeysers().begin(); iter != base->getGeysers().end(); iter++)
	{
		requestWriter->StartObject();
		requestWriter->Key("id");
		requestWriter->Int((*iter)->getID());

		requestWriter->Key("positionX");
		requestWriter->Int((*iter)->getInitialPosition().x);

		requestWriter->Key("positionY");
		requestWriter->Int((*iter)->getInitialPosition().y);

		requestWriter->Key("remaining");
		requestWriter->Int((*iter)->getResources());
		requestWriter->EndObject();
	}
	requestWriter->EndArray();

	requestWriter->EndObject();
}
// void serializeOccupiedRegions();

void MessageManager::serializeResources(BWAPI::Player player) {
	requestWriter->StartObject();
	requestWriter->Key("unitCount");
	requestWriter->Int(player->allUnitCount());

	requestWriter->Key("minerals");
	requestWriter->Int(player->minerals());

	requestWriter->Key("gas");
	requestWriter->Int(player->gas());

	requestWriter->Key("supplyUsed");
	requestWriter->Int(player->supplyUsed());

	requestWriter->Key("supplyTotal");
	requestWriter->Int(player->supplyTotal());
	requestWriter->EndObject();
}
