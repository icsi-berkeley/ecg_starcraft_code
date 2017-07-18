#include "ECGStarcraftManager.h"
#include "ECGUtil.h"
#include "BWTA.h"
#include "WorkerManager.h"
#include "ProductionManager.h"
#include "Message.h"
#include "MessageManager.h"

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

void ECGStarcraftManager::evaluateAction(Message* message)
{
  if (strcmp(message->readType(), "build") == 0)
    build(message);
  else if (strcmp(message->readType(), "gather") == 0)
    gather(message);
}

void ECGStarcraftManager::build(Message* message)
{
  BWAPI::UnitType unitType = message->readUnitType();
  int quantity = message->readQuantity();
  UnitDescriptor builder = message->readCommandedUnit(); // TODO: Actually use the builder
  // if (unitType is building) check for a location

  for (int i = 0; i < quantity; i++)
  {
    UAlbertaBot::ProductionManager::Instance().queueHighPriorityUnit(unitType);
  }
}

void ECGStarcraftManager::gather(Message* message)
{
  BWAPI::Unitset workers;

  UnitDescriptor commandedUnits = message->readCommandedUnit();
  Resource resourceType = message->readResourceType();

  if (commandedUnits.empty)
    workers = BWAPI::Broodwar->getUnitsInRadius(0, 0, 999999,
        BWAPI::Filter::GetType == BWAPI::UnitTypes::Terran_SCV && BWAPI::Filter::IsIdle && BWAPI::Filter::IsAlly);
  else
    workers = ECGUtil::resolveUnitDescriptor(commandedUnits);

  if (workers.empty())
    return;

  if (resourceType == Resource::minerals)
  {
    for (auto & worker : workers)
    {
      if (worker->canGather())
    		UAlbertaBot::WorkerManager::Instance().setMineralWorker(worker);
    }
  }
  else if (resourceType == Resource::gas)
  {
    for (auto & worker : workers)
    {
      BWAPI::Unit refinery = worker->getClosestUnit(BWAPI::Filter::IsRefinery && BWAPI::Filter::IsOwned);
      if (refinery && worker->canGather())
        UAlbertaBot::WorkerManager::Instance().workerData.setWorkerJob(worker, UAlbertaBot::WorkerData::Gas, refinery);
    }
  }
}
