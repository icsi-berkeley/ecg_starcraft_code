#pragma once

#include "Common.h"
#include "BuildingManager.h"
#include "WorkerData.h"

namespace UAlbertaBot
{
class Building;

class WorkerManager
{
    WorkerData  workerData;
    BWAPI::Unit previousClosestWorker;

    void        handleIdleWorkers();
    void        handleGasWorkers();
    void        handleMoveWorkers();
    void        handleCombatWorkers();
    void        handleRepairWorkers();

public:

    WorkerManager();

    void        update();
    void        onUnitDestroy(BWAPI::Unit unit);
    void        onUnitMorph(BWAPI::Unit unit);
    void        onUnitShow(BWAPI::Unit unit);
    void        finishedWithWorker(BWAPI::Unit unit);

    void        finishedWithCombatWorkers();

    void        drawResourceDebugInfo();
    void        updateWorkerStatus();
    void        drawWorkerInformation(int x,int y);

    int         getNumMineralWorkers();
    int         getNumGasWorkers();
    int         getNumIdleWorkers();
    void        setScoutWorker(BWAPI::Unit worker);

    bool        isWorkerScout(BWAPI::Unit worker) const;
    bool        isFree(BWAPI::Unit worker) const;
    bool        isBuilder(BWAPI::Unit worker) const;

    BWAPI::Unit getBuilder(Building & b,bool setJobAsBuilder = true);
    BWAPI::Unit getMoveWorker(BWAPI::Position p);
    BWAPI::Unit getClosestDepot(BWAPI::Unit worker);
    BWAPI::Unit getGasWorker(BWAPI::Unit refinery);
    BWAPI::Unit getClosestEnemyUnit(BWAPI::Unit worker);
    BWAPI::Unit getClosestMineralWorkerTo(BWAPI::Unit enemyUnit);

    void        setBuildingWorker(BWAPI::Unit worker,Building & b);
    void        setRepairWorker(BWAPI::Unit worker,BWAPI::Unit unitToRepair);
    void        stopRepairing(BWAPI::Unit worker);
    void        setMoveWorker(int m,int g,BWAPI::Position p);
    void        setCombatWorker(BWAPI::Unit worker);

    bool        willHaveResources(int mineralsRequired,int gasRequired,double distance);
    void        rebalanceWorkers();

    void        setMineralWorker(BWAPI::Unit unit); // Custom: made public
    void        setWorkerJob(BWAPI::Unit unit, WorkerData::WorkerJob job, BWAPI::Unit jobUnit); // Custom: added

};

}
