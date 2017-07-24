#pragma once
#include "BWAPI.h"
#include "ECGUtil.h"
#include "Message.h"

namespace ECGBot
{

enum EventType {ARMY, RESOURCE, CREATE};

struct Event {
  bool* boolResponse;
  Message* response;
  EventKind kind; // EventKind gives the trigger conditions whereas EventType gives the specific class of event
  EventType type; // Talk about confusing naming
  bool block;
  bool completed;

  Event(Message* r, bool* b, EventKind k)
  {
    assert(b != nullptr || r != nullptr); // At least one is not a nullptr
    assert(b == nullptr || r == nullptr); // At least one is a nullptr
    kind = k; boolResponse = b; response = r; block = false; completed = false;
  }
};

struct ArmyEvent : Event {
  UnitDescriptor units;

  ArmyEvent(UnitDescriptor ud, Message* r, EventKind k) : Event(r, nullptr, k)
  {
    units = ud; type = EventType::ARMY;
  }

  ArmyEvent(UnitDescriptor ud, bool* b, EventKind k) : Event(nullptr, b, k)
  {
    units = ud; type = EventType::ARMY;
  }
};

struct ResourceEvent : Event {
  Resource resource;
  int threshold;
  Comparator comparator;

  ResourceEvent(Resource re, int t, Comparator c, Message* r, EventKind k) : Event(r, nullptr, k)
  {
    resource = re; threshold = t; comparator = c; type = EventType::RESOURCE;
  }

  ResourceEvent(Resource re, int t, Comparator c, bool* b, EventKind k) : Event(nullptr, b, k)
  {
    resource = re; threshold = t; comparator = c; type = EventType::RESOURCE;
  }
};

struct CreateEvent : Event {
  int ecgID;
  int remainingCount;

  CreateEvent(int eid, int rc, bool* b, EventKind k) : Event(nullptr, b, k)
  {
    ecgID = eid; remainingCount = rc; type = EventType::CREATE;
  }
};

class EventManager
{
  EventManager();

  std::list<Event*>                eventList;

public:
  // singletons
  static EventManager & Instance();

  // TODO: Add register for events with boolean responses
  void        registerEvent(Message* message);
  void        registerArmyEvent(Message* event, Message* response, EventKind kind);
  void        registerResourceEvent(Message* event, Message* response, EventKind kind);
  void        registerCreateEvent(int ecgID, int quantity, bool* response, EventKind kind);

  void        checkEvent(Event* event);
  bool        checkArmyEvent(ArmyEvent* event);
  bool        checkResourceEvent(ResourceEvent* event);
  bool        checkCreateEvent(CreateEvent* event);

  void        update();
  void        onUnitComplete(BWAPI::Unit unit);

};
}
