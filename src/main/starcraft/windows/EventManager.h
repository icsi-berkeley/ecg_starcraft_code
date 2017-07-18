#pragma once
#include "BWAPI.h"
#include "ECGUtil.h"
#include "Message.h"

namespace ECGBot
{

struct ArmyEvent {
  UnitDescriptor units;
  Message* nextAction;
  EventKind kind;

  ArmyEvent(UnitDescriptor ud, Message* ac, EventKind ek);
};

struct ResourceEvent {
  Resource resource;
  int threshold;
  Comparator comparator;
  Message* nextAction;
  EventKind kind;

  ResourceEvent(Resource r, int t, Comparator c, Message* ac, EventKind ek);
};

struct LocationEvent {
  UnitDescriptor units;
  BWAPI::Position position;
  Region region;
  Message* nextAction;
  EventKind kind;

  LocationEvent(UnitDescriptor ud, BWAPI::Position pos, Region reg, Message* ac, EventKind ek);
};

// struct ConstructionEvent { TODO: Some magic shit to make this work
//   Message nextAction;
// };

class EventManager
{
  EventManager();

  std::list<ArmyEvent*>            armyEventList;
  std::list<ResourceEvent*>        resourceEventList;
  std::list<LocationEvent*>        locationEventList;
  // std::list<ConstructionEvent>  constructionEventList;

public:
  // singletons
  static EventManager & Instance();

  void        registerEvent(Message* message);
  void        registerArmyEvent(Message* event, Message* response, EventKind kind);
  void        registerResourceEvent(Message* event, Message* response, EventKind kind);
  void        registerLocationEvent(Message* event, Message* response, EventKind kind);

  bool        checkArmyEvent(ArmyEvent* event);
  bool        checkResourceEvent(ResourceEvent* event);
  bool        checkLocationEvent(LocationEvent* event);

  void        update();

};
}
