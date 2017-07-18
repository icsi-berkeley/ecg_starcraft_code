#include "Message.h"

using namespace ECGBot;

Message::Message()
{
  body = new rapidjson::Document();
}

Message::Message(const char* message)
{
  body = new rapidjson::Document();
  body->Parse(message);
  cursorPosition = BWAPI::Broodwar->getMousePosition();
  selectedUnits = BWAPI::Broodwar->getSelectedUnits();
}

Message::Message(const char* message, BWAPI::Position cursorPos, BWAPI::Unitset selected)
{
  body = new rapidjson::Document();
  body->Parse(message);
  cursorPosition = cursorPos;
  selectedUnits = selected;
}

Message::Message(const rapidjson::Value& msg)
{
  body = new rapidjson::Document();
  body->CopyFrom(msg, body->GetAllocator());
  cursorPosition = BWAPI::Broodwar->getMousePosition();
  selectedUnits = BWAPI::Broodwar->getSelectedUnits();
}

Message::Message(const rapidjson::Value& msg, BWAPI::Position cursorPos, BWAPI::Unitset selected)
{
  body = new rapidjson::Document();
  body->CopyFrom(msg, body->GetAllocator());
  cursorPosition = cursorPos;
  selectedUnits = selected;
}

void Message::setBody(const char* message)
{
  body->Parse(message);
}

void Message::resetGameInfo()
{
  cursorPosition = BWAPI::Broodwar->getMousePosition();
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

Message* Message::readEvent()
{
  return new Message((*body)["event"], cursorPosition, selectedUnits);
}

Message* Message::readResponse()
{
  return new Message((*body)["response"], cursorPosition, selectedUnits);
}

EventKind Message::readTrigger()
{
  EventKind triggerEnum;
  const char * triggerStr = (*body)["trigger"].GetString();

  if (strcmp(triggerStr, "ONCE") == 0)
    triggerEnum = EventKind::ONCE;
  else if (strcmp(triggerStr, "WHILE") == 0)
    triggerEnum = EventKind::WHILE;
  else if (strcmp(triggerStr, "UNTIL") == 0)
    triggerEnum = EventKind::UNTIL;
  else if (strcmp(triggerStr, "IF") == 0)
    triggerEnum = EventKind::IF;
  else
    assert(0); // Should not reach here

  return triggerEnum;
}

const char* Message::readType()
{
  return (*body)["type"].GetString();
}

int Message::readQuantity()
{
  return (*body)["quantity"].GetInt();
}

int Message::readThreshold()
{
  return (*body)["threshold"].GetInt();
}

BWAPI::UnitType Message::readUnitType()
{
  return ECGUtil::getUnitType((*body)["unit_type"].GetString());
}

Resource Message::readResourceType()
{
  Resource resourceEnum;
  const char * resourceStr = (*body)["resource_type"].GetString();

  if (strcmp(resourceStr, "minerals") == 0)
    resourceEnum = Resource::minerals;
  else if (strcmp(resourceStr, "gas") == 0)
    resourceEnum = Resource::gas;
  else if (strcmp(resourceStr, "supply") == 0)
    resourceEnum = Resource::supply;

  return resourceEnum;
  // return ECGUtil::getUnitType((*body)["resource_type"].GetString());
}

Resource Message::readResourceEnum()
{
  Resource resourceEnum;
  const char * resourceStr = (*body)["resource_enum"].GetString();

  if (strcmp(resourceStr, "minerals") == 0)
    resourceEnum = Resource::minerals;
  else if (strcmp(resourceStr, "gas") == 0)
    resourceEnum = Resource::gas;
  else if (strcmp(resourceStr, "supply") == 0)
    resourceEnum = Resource::supply;

  return resourceEnum;
}

Comparator Message::readComparator()
{
  Comparator compEnum;
  const char * compStr = (*body)["comparator"].GetString();

  if (strcmp(compStr, "GREATER") == 0)
    compEnum = Comparator::greater;
  else if (strcmp(compStr, "LESS") == 0)
    compEnum = Comparator::less;
  else if (strcmp(compStr, "EQUAL") == 0)
    compEnum = Comparator::equal;

  return compEnum;
}

UnitDescriptor Message::readCommandedUnit()
{
  if ((*body)["commanded_unit"].IsNull())
    return UnitDescriptor();
  return readUnitDescriptor((*body)["commanded_unit"]);
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
  int unitID;
  if (unitDescriptor["unit_name"].IsNull())
    unitID = -1;
  else
    unitID = NameManager::Instance().getUnitID(unitDescriptor["unit_name"].GetString());

  int ecgIdentifier = unitDescriptor["ecg_identifier"].GetInt(); // may want custom type for this
  int quantity = unitDescriptor["quantity"].GetInt();
  bool ally = unitDescriptor["ally"].GetBool();
  BWAPI::UnitType unitType = ECGUtil::getUnitType(unitDescriptor["unit_type"].GetString());

  UnitStatus statusEnum;
  const char * statusStr = unitDescriptor["status"].GetString();

  if (strcmp(statusStr, "IDLE") == 0)
    statusEnum = UnitStatus::IDLE;
  else if (strcmp(statusStr, "ATTACKING") == 0)
    statusEnum = UnitStatus::ATTACKING;
  else if (strcmp(statusStr, "DEFENDING") == 0)
    statusEnum = UnitStatus::DEFENDING;
  else if (strcmp(statusStr, "GATHERING") == 0)
    statusEnum = UnitStatus::GATHERING;
  else if (strcmp(statusStr, "EXPLORING") == 0)
    statusEnum = UnitStatus::EXPLORING;
  else if (strcmp(statusStr, "HARASSING") == 0)
    statusEnum = UnitStatus::HARASSING;
  else if (strcmp(statusStr, "BUILDING") == 0)
    statusEnum = UnitStatus::BUILDING;
  else if (strcmp(statusStr, "NA") == 0)
    statusEnum = UnitStatus::NA;

  if (unitDescriptor["location"].IsNull())
    return UnitDescriptor(unitID, ecgIdentifier, quantity, ally, unitType, statusEnum, selectedUnits);

  BWAPI::Position landmark = readLandmark(unitDescriptor["location"]);
  Region regionEnum = readRegion(unitDescriptor["location"]);

  return UnitDescriptor(unitID, ecgIdentifier, quantity, ally, unitType, statusEnum, selectedUnits, landmark, regionEnum);
}

BWAPI::Position Message::readLandmark()
{
  if ((*body)["location"].IsNull())
    return BWAPI::Position(0, 0);
  return readLandmark((*body)["location"]);
}

BWAPI::Position Message::readLandmark(const rapidjson::Value& locationDescriptor)
{
  BWAPI::Position landmark;

  if (locationDescriptor["landmark"].IsNull())
    return cursorPosition;
  else
    return ECGUtil::resolveUnitDescriptor(readUnitDescriptor(locationDescriptor["landmark"])).getPosition();
}

Region Message::readRegion()
{
  if ((*body)["location"].IsNull())
    assert(0); // SHOULD NEVER REACH THIS
  return readRegion((*body)["location"]);
}

Region Message::readRegion(const rapidjson::Value& locationDescriptor)
{
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

  return regionEnum;
}
