#include "ECGStarcraftManager.h"
#include "ECGUtil.h"
#include "Message.h"
#include "MessageManager.h"
#include "Global.h"
#include "EventManager.h"

using namespace ECGBot;

ECGStarcraftManager::ECGStarcraftManager()
{
  return;
}

ECGStarcraftManager & ECGStarcraftManager::Instance()
{
  static ECGStarcraftManager instance;
  return instance;
}

void ECGStarcraftManager::evaluateAction(Message* message, bool* blocking)
{
  if (strcmp(message->readType(), "build") == 0)
    build(message, blocking);
  else if (strcmp(message->readType(), "gather") == 0)
    gather(message, blocking);
  else if (strcmp(message->readType(), "move") == 0)
    move(message, blocking);
  else if (blocking != nullptr)
    *blocking = false;
}

void ECGStarcraftManager::build(Message* message, bool* blocking)
{
  BWAPI::UnitType unitType = message->readUnitType();
  int quantity = message->readQuantity();
  int ecgID = message->readECGID();

  for (int i = 0; i < quantity; i++)
    UAlbertaBot::Global::Production().queueLowPriorityUnit(unitType, ecgID);

  if (blocking != nullptr)
    EventManager::Instance().registerCreateEvent(ecgID, quantity, blocking, EventKind::ONCE);
}

void ECGStarcraftManager::gather(Message* message, bool* blocking)
{
  BWAPI::Unitset workers;

  UnitDescriptor commandedUnits = message->readCommandedUnit();
  Resource resourceType = message->readResourceType();

  if (commandedUnits.empty)
    workers = BWAPI::Broodwar->getUnitsInRadius(0, 0, 999999,
        BWAPI::Filter::GetType == BWAPI::UnitTypes::Terran_SCV && BWAPI::Filter::IsIdle && BWAPI::Filter::IsAlly);
  else
    workers = ECGUtil::resolveUnitDescriptor(commandedUnits, BWAPI::Filter::GetType == BWAPI::UnitTypes::Terran_SCV);

  // if (workers.empty())
  //   return;

  if (resourceType == Resource::MINERALS)
  {
    for (auto & worker : workers)
    {
      if (worker->canGather())
    		UAlbertaBot::Global::Workers().setMineralWorker(worker);
    }
  }
  else if (resourceType == Resource::GAS)
  {
    for (auto & worker : workers)
    {
      BWAPI::Unit refinery = worker->getClosestUnit(BWAPI::Filter::IsRefinery && BWAPI::Filter::IsOwned);
      if (refinery && worker->canGather())
        UAlbertaBot::Global::Workers().setWorkerJob(worker, UAlbertaBot::WorkerData::Gas, refinery);
    }
  }

  if (blocking != nullptr)
    *blocking = false;
}

void ECGStarcraftManager::move(Message* message, bool* blocking)
{
  UnitDescriptor commandedUnits = message->readCommandedUnit();
  BWAPI::Position landmark = message->readLandmark();
  Region region = message->readRegion();
  BWAPI::Unitset movers = ECGUtil::resolveUnitDescriptor(commandedUnits);

  BWAPI::Position destination = ECGUtil::resolveLocation(region, landmark);

  if (!movers.empty())
    movers.move(destination);

  // register boolean army event to make sure they moved
}
