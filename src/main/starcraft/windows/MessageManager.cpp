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
  rawMessage = new rapidjson::Document();
  request = new rapidjson::StringBuffer();
	currentMessage = new Message();
  requestWriter = new rapidjson::Writer<rapidjson::StringBuffer>(*request);
}

bool MessageManager::readIncoming()
{
  if (bridge->data_available((long) 0.02))
  {
    bridge->recv_json(rawMessage);

    // Check if it's for me
    if (!strcmp((*rawMessage)[0].GetString(), "SHOUT") && !strcmp((*rawMessage)[2].GetString(), "StarCraft"))
    {

      // Printing the message
      if ((size_t)rawMessage->Capacity() > 3)
      {
        BWAPI::Broodwar->sendText((*rawMessage)[1].GetString()); // Source
        BWAPI::Broodwar->sendText((*rawMessage)[2].GetString()); // Destination
        BWAPI::Broodwar->sendText((*rawMessage)[3].GetString()); // Raw Message
      }

			currentMessage->setBody((*rawMessage)[3].GetString()); // Unpack message
			currentMessage->resetGameInfo(); // Update associated game information
      return true;
    }
  }
  return false;
}

void MessageManager::beginOutgoing()
{
	request->Clear(); // Clear the previous request

  requestWriter->StartArray();
  requestWriter->String("SHOUT"); // Type of message
  requestWriter->String("StarCraft"); // Source of message
  requestWriter->String("FED1_ProblemSolver"); // Destination of message
  requestWriter->StartObject();
}

void MessageManager::sendOutgoing()
{
  requestWriter->EndObject();
  requestWriter->EndArray();

  BWAPI::Broodwar << request->GetString() << std::endl;
  bridge->send_string(request->GetString());
}

void MessageManager::sendStarted()
{
	beginOutgoing();
	requestWriter->Key("is_started");
	requestWriter->Bool(true);
	sendOutgoing();
}
