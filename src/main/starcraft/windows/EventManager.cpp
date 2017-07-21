#include "EventManager.h"
#include "Message.h"
#include "ECGStarcraftManager.h"

using namespace ECGBot;

ArmyEvent::ArmyEvent(UnitDescriptor ud, Message* ac, EventKind ek) {
  units = ud; nextAction = ac; kind = ek;
}

ResourceEvent::ResourceEvent(Resource r, int t, Comparator c, Message* ac, EventKind ek) {
  resource = r; threshold = t; comparator = c; nextAction = ac; kind = ek;
}

LocationEvent::LocationEvent(UnitDescriptor ud, BWAPI::Position pos, Region reg, Message* ac, EventKind ek) {
  units = ud; position = pos; region = reg; nextAction = ac; kind = ek;
}

EventManager::EventManager()
{
  return;
}

EventManager & EventManager::Instance()
{
  static EventManager instance;
  return instance;
}

void EventManager::registerEvent(Message* message)
{
  BWAPI::Broodwar->sendText("Registering event");
  Message* event = message->readEvent();
  Message* response = message->readResponse();
  EventKind kind = message->readTrigger();

  const char * type = event->readType();
  if (strcmp(type, "army") == 0)
    return registerArmyEvent(event, response, kind);
  else if (strcmp(type, "resource") == 0)
    return registerResourceEvent(event, response, kind);
  else if (strcmp(type, "location") == 0)
    return registerLocationEvent(event, response, kind);
  else if (strcmp(type, "construction") == 0)
    return; //registerConstructionEvent(event, response, kind);

  delete event;
  delete response;
}

void EventManager::registerArmyEvent(Message* event, Message* response, EventKind kind)
{
  BWAPI::Broodwar->sendText("Registering army event");
  ArmyEvent* newEvent = new ArmyEvent(event->readUnitDescriptor(), response, kind);
  armyEventList.push_back(newEvent);
  delete event;
}

void EventManager::registerResourceEvent(Message* event, Message* response, EventKind kind)
{
  BWAPI::Broodwar->sendText("Registering resource event");
  ResourceEvent* newEvent = new ResourceEvent(event->readResourceType(), event->readThreshold(), event->readComparator(), response, kind);
  resourceEventList.push_back(newEvent);
  delete event;
}

void EventManager::registerLocationEvent(Message* event, Message* response, EventKind kind)
{
  BWAPI::Broodwar->sendText("Registering location event");
  LocationEvent* newEvent = new LocationEvent(event->readUnitDescriptor(), event->readLandmark(), event->readRegion(), response, kind);
  locationEventList.push_back(newEvent);
  delete event;
}

bool EventManager::checkArmyEvent(ArmyEvent* event)
{
  BWAPI::Unitset matchedSet = ECGUtil::resolveUnitDescriptor(event->units);
  return !matchedSet.empty();
}

bool EventManager::checkResourceEvent(ResourceEvent* event)
{
  int value;
  switch (event->resource) {
    case Resource::SUPPLY:
      value = BWAPI::Broodwar->self()->supplyUsed();
      break;
    case Resource::MINERALS:
      value = BWAPI::Broodwar->self()->minerals();
      break;
    case Resource::GAS:
      value = BWAPI::Broodwar->self()->gas();
      break;
    default:
      return false;
  }

  switch(event->comparator) {
    case Comparator::GEQ:
      return value > event->threshold;
    case Comparator::LEQ:
      return value < event->threshold;
    case Comparator::EQ:
      return value == event->threshold;
  }
  return false;
}

bool EventManager::checkLocationEvent(LocationEvent* event)
{
  BWAPI::Unitset matchedSet = ECGUtil::resolveUnitDescriptor(event->units);

  // TODO: play around with reasonable radii
  switch (event->region) {
    case Region::EXACT:
      if (matchedSet.getPosition().getDistance(event->position) < 50.0) // is this reasonable? who knows????
        return true;
      break;
    case Region::CLOSE:
      if (matchedSet.getPosition().getDistance(event->position) < 100.0) // is this reasonable? who knows????
        return true;
      break;
    case Region::DISTANT:
      if (matchedSet.getPosition().getDistance(event->position) > 100.0) // is this reasonable? who knows????
        return true;
      break;
    case Region::RIGHT:
      if (matchedSet.getPosition().getDistance(event->position + BWAPI::Position(75, 0)) < 100.0) // is this reasonable? who knows????
        return true;
      break;
    case Region::LEFT:
      if (matchedSet.getPosition().getDistance(event->position - BWAPI::Position(75, 0)) < 100.0) // is this reasonable? who knows????
        return true;
      break;
    case Region::BACK:
      if (matchedSet.getPosition().getDistance(event->position + BWAPI::Position(0, 75)) < 100.0) // is this reasonable? who knows????
        return true;
      break;
    case Region::FRONT:
      if (matchedSet.getPosition().getDistance(event->position - BWAPI::Position(75, 0)) < 100.0) // is this reasonable? who knows????
        return true;
      break;
  }
  return false;
}


void EventManager::update()
{
  for (auto iter = armyEventList.begin(); iter != armyEventList.end();)
  {
    if (checkArmyEvent(*iter))
    {

      ECGStarcraftManager::Instance().evaluateAction((*iter)->nextAction);
      if ((*iter)->kind == EventKind::ONCE || (*iter)->kind == EventKind::UNTIL)
      {
        delete (*iter)->nextAction;
        delete *iter;
        armyEventList.erase(iter++);
      }
      else
        iter++;
    }
    else
    {
      if ((*iter)->kind == EventKind::WHILE)
      {
        delete (*iter)->nextAction;
        delete *iter;
        armyEventList.erase(iter++);
      }
      else
        iter++;
    }
  }

  // TODO: Add the other events
}
