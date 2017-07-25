#include "Message.h"

using namespace ECGBot;

Message::Message()
{
  body = new rapidjson::Document();
}

Message::Message(const char* message)
{
  body = new rapidjson::Document();
  setBody(message);
  cursorPosition = ECGUtil::getMousePosition();
  selectedUnits = BWAPI::Broodwar->getSelectedUnits();
}

Message::Message(const char* message, BWAPI::Position cursorPos, BWAPI::Unitset selected)
{
  body = new rapidjson::Document();
  setBody(message);
  cursorPosition = cursorPos;
  selectedUnits = selected;
}

Message::Message(const rapidjson::Value& msg)
{
  body = new rapidjson::Document();
  body->CopyFrom(msg, body->GetAllocator());
  cursorPosition = ECGUtil::getMousePosition();
  selectedUnits = BWAPI::Broodwar->getSelectedUnits();
  assert((*body)["type"].IsString());
}

Message::Message(const rapidjson::Value& msg, BWAPI::Position cursorPos, BWAPI::Unitset selected)
{
  body = new rapidjson::Document();
  body->CopyFrom(msg, body->GetAllocator());
  cursorPosition = cursorPos;
  selectedUnits = selected;
  assert((*body)["type"].IsString());
}

void Message::setBody(const char* message)
{
  body->Parse(message);
  assert((*body)["type"].IsString());
}

void Message::resetGameInfo()
{
  cursorPosition = ECGUtil::getMousePosition();
  selectedUnits = BWAPI::Broodwar->getSelectedUnits();
}

bool Message::isStarted()
{
  return strcmp((*body)["type"].GetString(), "is_started") == 0;
}

bool Message::isConditional()
{
  return strcmp((*body)["type"].GetString(), "conditional") == 0;
}

bool Message::isSequential()
{
  return strcmp((*body)["type"].GetString(), "sequential") == 0;
}

bool Message::isAction()
{
  return !isConditional() && !isSequential();
}

const char* Message::readType()
{
  return (*body)["type"].GetString();
}

EventKind Message::readTrigger()
{
  assert((*body)["trigger"].IsString());
  EventKind triggerEnum;
  const char * triggerStr = (*body)["trigger"].GetString();

  if (strcmp(triggerStr, "ONCE") == 0)
    triggerEnum = EventKind::ONCE;
  else if (strcmp(triggerStr, "WHILE") == 0)
    triggerEnum = EventKind::WHILE;
  else if (strcmp(triggerStr, "UNTIL") == 0)
    triggerEnum = EventKind::UNTIL;
  else if (strcmp(triggerStr, "ALWAYS") == 0)
    triggerEnum = EventKind::ALWAYS;
  else
    assert(0); // Should not reach here

  return triggerEnum;
}

Message* Message::readEvent()
{
  assert((*body)["event"].IsObject());
  return new Message((*body)["event"], cursorPosition, selectedUnits);
}

Message* Message::readResponse()
{
  assert((*body)["response"].IsObject());
  return new Message((*body)["response"], cursorPosition, selectedUnits);
}

Message* Message::readFirst()
{
  assert((*body)["first"].IsObject());
  return new Message((*body)["first"], cursorPosition, selectedUnits);
}

Message* Message::readSecond()
{
  assert((*body)["second"].IsObject());
  return new Message((*body)["second"], cursorPosition, selectedUnits);
}

int Message::readQuantity()
{
  assert((*body)["quantity"].IsInt());
  return (*body)["quantity"].GetInt();
}

BWAPI::UnitType Message::readUnitType()
{
  return ECGUtil::getUnitType((*body)["unit_type"].GetString());
}

int Message::readECGID()
{
  assert((*body)["quantity"].IsInt());
  return (*body)["ecg_id"].GetInt();
}

Resource Message::readResourceType()
{
  assert((*body)["resource_type"].IsString());
  Resource resourceEnum;
  const char * resourceStr = (*body)["resource_type"].GetString();

  if (strcmp(resourceStr, "MINERALS") == 0)
    resourceEnum = Resource::MINERALS;
  else if (strcmp(resourceStr, "GAS") == 0)
    resourceEnum = Resource::GAS;
  else if (strcmp(resourceStr, "SUPPLY") == 0)
    resourceEnum = Resource::SUPPLY;
  else
    assert(0);

  return resourceEnum;
}

int Message::readThreshold()
{
  assert((*body)["threshold"].IsInt());
  return (*body)["threshold"].GetInt();
}

Comparator Message::readComparator()
{
  return readComparator((*body)["comparator"].GetString());
}

Comparator Message::readComparator(const char * compStr)
{
  assert(compStr != NULL);
  Comparator compEnum;

  if (strcmp(compStr, "GEQ") == 0)
    compEnum = Comparator::GEQ;
  else if (strcmp(compStr, "LEQ") == 0)
    compEnum = Comparator::LEQ;
  else if (strcmp(compStr, "EQ") == 0)
    compEnum = Comparator::EQ;
  else
    assert(0);

  return compEnum;
}

UnitDescriptor Message::readCommandedUnit()
{
  if ((*body)["commanded_unit"].IsNull())
    return UnitDescriptor();
  return readUnitDescriptor((*body)["commanded_unit"]);
}

UnitDescriptor Message::readSetOne()
{
  if ((*body)["set_one"].IsNull())
    return UnitDescriptor();
  return readUnitDescriptor((*body)["set_one"]);
}

UnitDescriptor Message::readSetTwo()
{
  if ((*body)["set_two"].IsNull())
    return UnitDescriptor();
  return readUnitDescriptor((*body)["set_two"]);
}

UnitDescriptor Message::readUnitDescriptor()
{
  if ((*body)["unit_descriptor"].IsNull())
    return UnitDescriptor();
  return readUnitDescriptor((*body)["unit_descriptor"]);
}

UnitDescriptor Message::readUnitDescriptor(const rapidjson::Value& unitDescriptor)
{
  // TODO: I think I should put a flag on the unit_descriptor saying if I have unresolved anaphora
  // If true, then fill a field on the UnitDescriptor containing a set of the selected units
  // OR, I can always try using the selectedUnits and if they are empty then i try again without them < doing that

  int unitID = unitDescriptor["name"].IsNull() ? 0 : NameManager::Instance().getUnitID(unitDescriptor["name"].GetString());
  int ecgID = unitDescriptor["ecg_id"].IsNull() ? 0 : unitDescriptor["ecg_id"].GetInt(); // may want custom type for this

  int quantity = unitDescriptor["quantity"].GetInt();
  Comparator compEnum = readComparator(unitDescriptor["comparator"].GetString());

  bool ally = unitDescriptor["ally"].GetBool();
  BWAPI::UnitType unitType = ECGUtil::getUnitType(unitDescriptor["unit_type"].GetString());

  UnitStatus statusEnum;
  const char * statusStr = unitDescriptor["status"].GetString();

  if (strcmp(statusStr, "IDLE") == 0)
    statusEnum = UnitStatus::IDLE;
  else if (strcmp(statusStr, "GATHERING") == 0)
    statusEnum = UnitStatus::GATHERING;
  else if (strcmp(statusStr, "BUILDING") == 0)
    statusEnum = UnitStatus::BUILDING;
  else if (strcmp(statusStr, "UNDERATTACK") == 0)
    statusEnum = UnitStatus::UNDERATTACK;
  else if (strcmp(statusStr, "ATTACKING") == 0)
    statusEnum = UnitStatus::ATTACKING;
  else if (strcmp(statusStr, "DEFENDING") == 0)
    statusEnum = UnitStatus::DEFENDING;
  else if (strcmp(statusStr, "NA") == 0)
    statusEnum = UnitStatus::NA;
  else
    assert(0);

  BWAPI::Position landmark = readLandmark(unitDescriptor["location"]);
  Region regionEnum = readRegion(unitDescriptor["location"]);

  return UnitDescriptor(unitID, ecgID, quantity, compEnum, ally, unitType, statusEnum, selectedUnits, landmark, regionEnum);
}

BWAPI::Position Message::readLandmark()
{
  if ((*body)["location"].IsNull())
    return BWAPI::Positions::None;
  return readLandmark((*body)["location"]);
}

BWAPI::Position Message::readLandmark(const rapidjson::Value& locationDescriptor)
{
  if (locationDescriptor.IsNull())
    return BWAPI::Positions::None;
  else if (locationDescriptor["landmark"].IsNull())
    return cursorPosition;
  else
    return ECGUtil::resolveUnitDescriptor(readUnitDescriptor(locationDescriptor["landmark"])).getPosition();
}

Region Message::readRegion()
{
  if ((*body)["location"].IsNull())
    return Region::EXACT;
  return readRegion((*body)["location"]);
}

Region Message::readRegion(const rapidjson::Value& locationDescriptor)
{
  if (locationDescriptor.IsNull())
    return Region::EXACT;

  assert(locationDescriptor["region"].IsString());
  const char* regionStr = locationDescriptor["region"].GetString();

  Region regionEnum;

  if (strcmp(regionStr, "EXACT") == 0)
    regionEnum = Region::EXACT;
  else if (strcmp(regionStr, "CLOSE") == 0)
    regionEnum = Region::CLOSE;
  else if (strcmp(regionStr, "DISTANT") == 0)
    regionEnum = Region::DISTANT;
  else if (strcmp(regionStr, "RIGHT") == 0)
    regionEnum = Region::RIGHT;
  else if (strcmp(regionStr, "LEFT") == 0)
    regionEnum = Region::LEFT;
  else if (strcmp(regionStr, "BACK") == 0)
    regionEnum = Region::BACK;
  else if (strcmp(regionStr, "FRONT") == 0)
    regionEnum = Region::FRONT;
  else
    assert(0);

  return regionEnum;
}
