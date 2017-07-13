#include "ECGStarcraftManager.h"
#include "ECGUtil.h"
#include "BWTA.h"

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

bool ECGStarcraftManager::build(Units commandedUnit, BWAPI::UnitType unitType, int count)
{
  for (int i = 0; i < count; i++)
  {
    UAlbertaBot::ProductionManager::Instance().queueHighPriorityUnit(unitType);
  }
  return true;
}

bool ECGStarcraftManager::gather(Units commandedUnit, BWAPI::UnitType resourceType)
{
  if (resourceType == BWAPI::UnitTypes::Resource_Mineral_Field)
  {
    for (auto & worker : UAlbertaBot::WorkerManager::Instance().workerData.getWorkers())
    {
      if (UAlbertaBot::WorkerManager::Instance().workerData.getWorkerJob(worker) == UAlbertaBot::WorkerData::Idle)
      {
        UAlbertaBot::WorkerManager::Instance().setMineralWorker(worker);
      }
    }
    return true;
  }
  else if (resourceType == BWAPI::UnitTypes::Resource_Vespene_Geyser)
  {
    for (auto & worker : UAlbertaBot::WorkerManager::Instance().workerData.getWorkers())
    {
      if (UAlbertaBot::WorkerManager::Instance().workerData.getWorkerJob(worker) == UAlbertaBot::WorkerData::Idle)
      {
        BWAPI::Unit refinery = worker->getClosestUnit(BWAPI::Filter::IsRefinery && BWAPI::Filter::IsOwned);
        if (refinery)
        {
          UAlbertaBot::WorkerManager::Instance().workerData.setWorkerJob(worker, UAlbertaBot::WorkerData::Gas, refinery);
        }
        else
        {
          // TODO: Make this unit build a refinery and then start harvesting from it
        }
      }
    }
  }
}
