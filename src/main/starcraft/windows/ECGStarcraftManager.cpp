#include "ECGStarcraftManager.h"
#include "ECGUtil.h"
#include "Message.h"
#include "MessageManager.h"
#include "Global.h"

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
  else if (strcmp(message->readType(), "move") == 0)
    move(message);
}

void ECGStarcraftManager::build(Message* message)
{
  BWAPI::UnitType unitType = message->readUnitType();
  int quantity = message->readQuantity();

  for (int i = 0; i < quantity; i++)
    UAlbertaBot::Global::Production().queueLowPriorityUnit(unitType); //TODO:albertanewversionfix
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
    workers = ECGUtil::resolveUnitDescriptor(commandedUnits, BWAPI::Filter::GetType == BWAPI::UnitTypes::Terran_SCV);

  if (workers.empty())
    return;

  if (resourceType == Resource::MINERALS)
  {
    for (auto & worker : workers)
    {
      if (worker->canGather())
    		UAlbertaBot::Global::Workers().setMineralWorker(worker); //TODO:albertanewversionfix
    }
  }
  else if (resourceType == Resource::GAS)
  {
    for (auto & worker : workers)
    {
      BWAPI::Unit refinery = worker->getClosestUnit(BWAPI::Filter::IsRefinery && BWAPI::Filter::IsOwned);
      if (refinery && worker->canGather())
        UAlbertaBot::Global::Workers().setWorkerJob(worker, UAlbertaBot::WorkerData::Gas, refinery); //TODO:albertanewversionfix
    }
  }
}

void ECGStarcraftManager::move(Message* message)
{
  UnitDescriptor commandedUnits = message->readCommandedUnit();
  BWAPI::Position destination = message->readLandmark();
  BWAPI::Unitset movers = ECGUtil::resolveUnitDescriptor(commandedUnits);

  if (!movers.empty())
    movers.move(destination);
}
