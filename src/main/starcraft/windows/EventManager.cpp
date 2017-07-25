#include "EventManager.h"
#include "Message.h"
#include "ECGStarcraftManager.h"

using namespace ECGBot;

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

  delete event;
  delete response;
}

void EventManager::registerArmyEvent(Message* event, Message* response, EventKind kind)
{
  BWAPI::Broodwar->sendText("Registering army event");
  ArmyEvent* newEvent = new ArmyEvent(event->readUnitDescriptor(), response, kind);
  eventList.push_back(newEvent);
  delete event;
}

void EventManager::registerResourceEvent(Message* event, Message* response, EventKind kind)
{
  BWAPI::Broodwar->sendText("Registering resource event");
  ResourceEvent* newEvent = new ResourceEvent(event->readResourceType(), event->readThreshold(), event->readComparator(), response, kind);
  eventList.push_back(newEvent);
  delete event;
}

void EventManager::registerCreateEvent(int ecgID, int quantity, bool* response, EventKind kind)
{
  BWAPI::Broodwar->sendText("Registering create event");
  CreateEvent* newEvent = new CreateEvent(ecgID, quantity, response, kind);
  eventList.push_back(newEvent);
}

bool* EventManager::registerSequentialEvent(Message* response, bool* secondIncomplete)
{
  BWAPI::Broodwar->sendText("Registering sequential event");
  SequentialEvent* newEvent = new SequentialEvent(response, secondIncomplete);
  eventList.push_back(newEvent);
  return &(newEvent->block);
}

bool EventManager::checkArmyEvent(ArmyEvent* event)
{
  BWAPI::Unitset matchedSet = ECGUtil::resolveUnitDescriptor(event->units);
  return !matchedSet.empty();
}

bool EventManager::checkResourceEvent(ResourceEvent* event)
{
  int value;
  int threshold = event->threshold;
  switch (event->resource) {
    case Resource::SUPPLY:
      value = BWAPI::Broodwar->self()->supplyUsed();
      threshold = threshold < 0 ? BWAPI::Broodwar->self()->supplyTotal() + threshold + 1 : threshold;
      // BWAPI::Broodwar->sendText("Supply Value: %d, Supply Threshold: %d", value, threshold);
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
      return value >= threshold;
    case Comparator::LEQ:
      return value <= threshold;
    case Comparator::EQ:
      return value == threshold;
  }
  return false;
}

bool EventManager::checkCreateEvent(CreateEvent* event)
{
  return event->remainingCount <= 0;
}

bool EventManager::checkSequentialEvent(SequentialEvent* event)
{
  return true;
}

void EventManager::update()
{
  for (auto eventIter = eventList.begin(); eventIter != eventList.end();)
  {
    Event* event = *eventIter;
    if (event->block)
    {
      eventIter++;
    }
    else if (event->completed)
    {
      if (event->type == EventType::SEQUENTIAL && ((SequentialEvent*) event)->secondIncomplete != nullptr)
        *(((SequentialEvent*) event)->secondIncomplete) = false;
      delete event->response;
      delete event;
      eventList.erase(eventIter++);
    }
    else
    {
      eventIter++;
      bool result = false;
      switch (event->type)
      {
        case EventType::ARMY:
          result = checkArmyEvent((ArmyEvent*) event);
          break;
        case EventType::RESOURCE:
          result = checkResourceEvent((ResourceEvent*) event);
          break;
        case EventType::CREATE:
          result = checkCreateEvent((CreateEvent*) event);
          break;
        case EventType::SEQUENTIAL:
          result = checkSequentialEvent((SequentialEvent*) event);
          break;
      }

      if (result && event->kind == EventKind::UNTIL)
      {
        event->completed = true;
      }
      else if (result || event->kind == EventKind::UNTIL)
      {
        if (event->kind == EventKind::ONCE)
          event->completed = true;

        if (event->boolResponse == nullptr)
        {
          event->block = true;
          ECGStarcraftManager::Instance().evaluateAction(event->response, &(event->block));
        }
        else
        {
          *(event->boolResponse) = false;
          event->block = false;
        }
      }
      else if (event->kind == EventKind::WHILE)
      {
        event->completed = true;
      }
    }
  }
}

void EventManager::onUnitComplete(BWAPI::Unit unit)
{
  int eid = NameManager::Instance().getECGID(unit->getID());
  for (auto& event : eventList)
    if (event->type == EventType::CREATE)
      if (((CreateEvent*) event)->ecgID == eid)
        ((CreateEvent*) event)->remainingCount--;
}
