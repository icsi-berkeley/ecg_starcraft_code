#pragma once
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "TransportBridge.h"
#include "BWAPI.h"

namespace ECGBot
{

enum UnitsStatus {IDLE, ATTACKING, DEFENDING, GATHERING, EXPLORING, HARASSING, BUILDING, NA};
enum LocationPosition {EXACT, NEAR, FAR, RIGHT, LEFT, BACK, FRONT};

struct Location;

struct Units {
  int quantity;
  int unitID;
  BWAPI::UnitType unitType;
  int unitIdentifier; // Not sure this is needed. Used to identify a unit which does not have an ID yet
  Location location;
  UnitsStatus status;
};

struct Location {
  Units landmark;
  LocationPosition position;
}

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

    void              beginMessage();
    void              sendMessage();
    bool              readMessage();

    bool              isStarted();
    bool              isConditional();
    bool              isSequential();
    bool              isAction();

    const char*		    readType();
    BWAPI::UnitType   readUnitType();
    int               readQuantity();

    void              sendStarted();

};
}
