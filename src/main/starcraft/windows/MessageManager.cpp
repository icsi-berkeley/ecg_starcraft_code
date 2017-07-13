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

void MessageManager::beginMessage()
{
	request->Clear(); // Clear the previous request

  requestWriter->StartArray();
  requestWriter->String("SHOUT"); // Type of message
  requestWriter->String("StarCraft"); // Source of message
  requestWriter->String("FED1_ProblemSolver"); // Destination of message
  requestWriter->StartObject();
}

void MessageManager::sendMessage()
{
  requestWriter->EndObject();
  requestWriter->EndArray();

  BWAPI::Broodwar << request->GetString() << std::endl;
  bridge->send_string(request->GetString());
}

bool MessageManager::readMessage()
{
  if (bridge->data_available((long) 0.02))
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

bool MessageManager::isStarted()
{
	return strcmp((*response)["type"].GetString(), "is_started") == 0;
}

bool MessageManager::isConditional()
{
	return strcmp((*response)["type"].GetString(), "conditional") == 0;
}

bool MessageManager::isSequential()
{
	return strcmp((*response)["type"].GetString(), "sequential") == 0;
}

bool MessageManager::isAction()
{
	return !isConditional() && !isSequential();
}

const char* MessageManager::readType()
{
	return (*response)["action"].GetString();
}

BWAPI::UnitType MessageManager::readUnitType()
{
	return ECGUtil::getUnitType((*response)["unit_type"].GetString());
}

int MessageManager::readQuantity()
{
	return (*response)["quantity"].GetInt();
}

void MessageManager::sendStarted()
{
	beginMessage();
	requestWriter->Key("is_started");
	requestWriter->Bool(true);
	sendMessage();
}
