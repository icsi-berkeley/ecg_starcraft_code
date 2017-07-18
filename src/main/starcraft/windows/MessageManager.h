#pragma once
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "TransportBridge.h"
#include "BWAPI.h"
#include "Message.h"

namespace ECGBot
{

class MessageManager
{
    MessageManager();

    TransportBridge*                            bridge;
    rapidjson::Document*                        rawMessage;
    rapidjson::StringBuffer*                    request;
    rapidjson::Writer<rapidjson::StringBuffer>* requestWriter;
    Message*                                    currentMessage;

    void          initializeTransport();

public:

    // singletons
    static MessageManager & Instance();

    bool              readIncoming();
    Message*          current() { return currentMessage; }

    void              beginOutgoing();
    void              sendOutgoing();
    void              sendStarted();

};
}
