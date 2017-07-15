#pragma once
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "BWAPI.h"
#include "MessageManager.h"

namespace ECGBot
{

enum Resource {minerals, gas, population};
enum Comparator {greater, less, equal};
enum EventKind {ONCE, WHILE, UNTIL, IF};

struct ArmyEvent {
  UnitDescriptor units;
  Message nextAction;
  EventKind kind;
  BWAPI::Position initialMousePosition;
  BWAPI::Unitset selectedUnits;
};

struct ResourceEvent {
  Resource resource;
  int threshold;
  Comparator comparator;
  Message nextAction;
  EventKind kind;
  BWAPI::Position initialMousePosition;
  BWAPI::Unitset selectedUnits;
}

struct LocationEvent {
  UnitDescriptor units;
  BWAPI::Position position;
  Message nextAction;
  EventKind kind;
  BWAPI::Position initialMousePosition;
  BWAPI::Unitset selectedUnits;
};

// struct ConstructionEvent { TODO: Some magic shit to make this work
//   Message nextAction;
// };

class EventManager
{
    EventManager();

    std::list<ArmyEvent>          armyEventList;
    std::list<ResourceEvent>      resourceEventList;
    std::list<LocationEvent>      locationEventList;

public:
    // singletons
    static EventManager & Instance();

};
}
