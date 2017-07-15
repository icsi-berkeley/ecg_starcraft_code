#pragma once
#include "rapidjson/document.h"
#include "BWAPI.h"
#include "ECGUtil.h"

namespace ECGBot
{

class Message
{
  rapidjson::Document*    body;

public:
  Message();
  Message(const char* message);

  void				            setBody(const char* message);

  bool                    isStarted();
  bool                    isConditional();
  bool                    isSequential();
  bool                    isAction();

  const char*		          readType();
  int                     readQuantity();
  BWAPI::UnitType         readUnitType();
  BWAPI::UnitType         readResourceType();
  UnitDescriptor          readCommandedUnit();
  UnitDescriptor          readUnitDescriptor();
  UnitDescriptor          readUnitDescriptor(rapidjson::Value unitDescriptor);
  BWAPI::Position         readLocation();
  BWAPI::Position         readLocation(rapidjson::Value locationDescriptor);
  Region                  readRegion(rapidjson::Value locationDescriptor);
  Region                  readRegion(const char* regionStr);
  BWAPI::Position         readTarget(rapidjson::Value descriptor); // TODO: Implement this

};

}
