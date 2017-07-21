#pragma once
#include "rapidjson/document.h"
#include "BWAPI.h"
#include "ECGUtil.h"
#include "NameManager.h"

namespace ECGBot
{

class Message
{
  rapidjson::Document*    body;
  BWAPI::Position         cursorPosition;
  BWAPI::Unitset          selectedUnits;

public:
  Message();
  Message(const char* message);
  Message(const char* message, BWAPI::Position cursorPos, BWAPI::Unitset selected);
  Message(const rapidjson::Value& msg);
  Message(const rapidjson::Value& msg, BWAPI::Position cursorPos, BWAPI::Unitset selected);

  void				            setBody(const char* message);
  void                    resetGameInfo();

  bool                    isStarted();
  bool                    isConditional();
  bool                    isSequential();
  bool                    isAction();

  const char*		          readType();

  EventKind               readTrigger();
  Message*                readEvent();
  Message*                readResponse();

  Message*                readFirst();
  Message*                readSecond();

  int                     readQuantity();
  BWAPI::UnitType         readUnitType();
  int                     readECGID();

  Resource                readResourceType();
  int                     readThreshold();
  Comparator              readComparator();
  Comparator              readComparator(const char * compStr);

  UnitDescriptor          readCommandedUnit();
  UnitDescriptor          readSetOne();
  UnitDescriptor          readSetTwo();
  UnitDescriptor          readUnitDescriptor();
  UnitDescriptor          readUnitDescriptor(const rapidjson::Value& unitDescriptor);

  BWAPI::Position         readLandmark();
  BWAPI::Position         readLandmark(const rapidjson::Value& locationDescriptor);
  Region                  readRegion();
  Region                  readRegion(const rapidjson::Value& locationDescriptor);


  BWAPI::Position         readTarget(const rapidjson::Value& descriptor); // TODO: Implement this

};

}
