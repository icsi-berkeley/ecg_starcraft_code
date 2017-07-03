#pragma once
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "TransportBridge.h"
#include "BWAPI.h"

namespace ECGBot
{

class MessageManager
{
    MessageManager();

    TransportBridge*                            bridge;
    rapidjson::Document*                        message;
    rapidjson::Document*                        response;
    rapidjson::StringBuffer*                    request;
    rapidjson::Writer<rapidjson::StringBuffer>* requestWriter;

    void          initializeTransport();

public:

    // singletons
    static MessageManager & Instance();

    void          sendRequest();
    bool          readResponse();

    void          serializeInformation();
	void          serializePlayer(BWAPI::Player player);
	void          serializeUnits(BWAPI::Player player);
    void          serializeBaseLocation(BWAPI::Player player);
    void          serializeOccupiedRegions();
    void          serializeResources(BWAPI::Player player);

};
}
