#include "ECGStarcraftManager.h"
#include "ECGUtil.h"
#include "BWTA.h"
#include "WorkerManager.h"
#include "ProductionManager.h"

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

void ECGStarcraftManager::build(UnitDescriptor commandedUnits, const BWAPI::UnitType unitType, int count)
{
  for (int i = 0; i < count; i++)
  {
    UAlbertaBot::ProductionManager::Instance().queueHighPriorityUnit(unitType);
  }
  // return true;
}

void ECGStarcraftManager::gather(UnitDescriptor commandedUnits, const BWAPI::UnitType resourceType)
{
	BWAPI::Unitset workers = ECGUtil::resolveUnitDescriptor(commandedUnits);
	if (workers.empty())
		workers = BWAPI::Broodwar->getUnitsInRadius(0, 0, 999999,
			BWAPI::Filter::GetType == BWAPI::UnitTypes::Terran_SCV && BWAPI::Filter::IsIdle && BWAPI::Filter::IsAlly);
			// TODO: I need to do something about getting selected units

  if (resourceType == BWAPI::UnitTypes::Resource_Mineral_Field)
  {
    for (auto & worker : workers)
    	UAlbertaBot::WorkerManager::Instance().setMineralWorker(worker);
  }
  else if (resourceType == BWAPI::UnitTypes::Resource_Vespene_Geyser)
  {
    for (auto & worker : workers)
    {
      BWAPI::Unit refinery = worker->getClosestUnit(BWAPI::Filter::IsRefinery && BWAPI::Filter::IsOwned);
      if (refinery)
	      UAlbertaBot::WorkerManager::Instance().workerData.setWorkerJob(worker, UAlbertaBot::WorkerData::Gas, refinery);
    }
  }
}
