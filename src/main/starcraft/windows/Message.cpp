#include "Message.h"
#include "NameManager.h"

using namespace ECGBot;

Message::Message()
{
	body = new rapidjson::Document();
}

Message::Message(const char* message)
{
	body = new rapidjson::Document();
	body->Parse(message);
}

void Message::setBody(const char* message)
{
	body->Parse(message);
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

int Message::readQuantity()
{
	return (*body)["quantity"].GetInt();
}

BWAPI::UnitType Message::readUnitType()
{
	return ECGUtil::getUnitType((*body)["unit_type"].GetString());
}

BWAPI::UnitType Message::readResourceType()
{
  return ECGUtil::getUnitType((*body)["resource_type"].GetString());
}

UnitDescriptor Message::readCommandedUnit()
{
  if ((*body)["commanded_unit"].IsNull())
    return UnitDescriptor();
  return readUnitDescriptor((*body)["commanded_unit"].GetObject());
}

UnitDescriptor Message::readUnitDescriptor()
{
  if ((*body)["unit_descriptor"].IsNull())
    return UnitDescriptor();
  return readUnitDescriptor((*body)["unit_descriptor"].GetObject());
}

UnitDescriptor Message::readUnitDescriptor(rapidjson::Value unitDescriptor)
{
  int unitID;
  if (unitDescriptor["unit_name"].IsNull())
    unitID = 0;
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
    return UnitDescriptor(unitID, ecgIdentifier, quantity, ally, unitType, statusEnum);


  BWAPI::Position landmark= readLocation(unitDescriptor["location"].GetObject());
  Region regionEnum = readRegion(unitDescriptor["location"].GetObject());

  return UnitDescriptor(unitID, ecgIdentifier, quantity, ally, unitType, statusEnum, landmark, regionEnum);
}

BWAPI::Position Message::readLocation()
{
  if ((*body)["location"].IsNull())
    return BWAPI::Position(0, 0);
  return readLocation((*body)["location"].GetObject());
}

BWAPI::Position Message::readLocation(rapidjson::Value locationDescriptor)
{
  // TODO: FINISH THIS
  BWAPI::Position landmark;

  if (locationDescriptor["landmark"].IsNull())
    landmark = BWAPI::Broodwar->getMousePosition();
  else
    landmark = ECGUtil::resolveUnitDescriptor(readUnitDescriptor(locationDescriptor["landmark"].GetObject())).getPosition();

  Region regionEnum = readRegion(locationDescriptor["region"].GetString());  // TODO: DO SOMETHING WITH THE regionEnum

  return landmark;
}

Region Message::readRegion(rapidjson::Value locationDescriptor)
{
  return readRegion(locationDescriptor["region"].GetString());
}

Region Message::readRegion(const char* regionStr)
{
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
