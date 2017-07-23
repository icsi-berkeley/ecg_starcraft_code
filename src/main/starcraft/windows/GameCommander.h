#pragma once

#include "Common.h"
#include "CombatCommander.h"
#include "ProductionManager.h"
#include "ScoutManager.h"
#include "Timer.hpp"

namespace UAlbertaBot
{

class UAlbertaBot_Tournament;

class GameCommander
{
    Timer                       _timer;
    CombatCommander             _combatCommander;
    ProductionManager           _productionManager;
    ScoutManager                _scoutManager;

    std::vector<BWAPI::Unit>    _validUnits;
    std::vector<BWAPI::Unit>    _combatUnits;
    std::vector<BWAPI::Unit>    _scoutUnits;

    bool                        _initialScoutSet;

    void assignUnit(BWAPI::Unit unit,std::vector<BWAPI::Unit> & set);
    bool isAssigned(BWAPI::Unit unit) const;

public:

    GameCommander();

    void onStart();
    void update();

    void handleUnitAssignments();
    void setValidUnits();
    void setScoutUnits();
    void setCombatUnits();

    void drawDebugInterface();
    void drawGameInformation(int x,int y);

    BWAPI::Unit getFirstSupplyProvider();
    BWAPI::Unit getClosestUnitToTarget(BWAPI::UnitType type,BWAPI::Position target);
    BWAPI::Unit getClosestWorkerToTarget(BWAPI::Position target);

    ProductionManager & getProductionManager();

    void onUnitShow(BWAPI::Unit unit);
    void onUnitHide(BWAPI::Unit unit);
    void onUnitCreate(BWAPI::Unit unit);
    void onUnitComplete(BWAPI::Unit unit);
    void onUnitRenegade(BWAPI::Unit unit);
    void onUnitDestroy(BWAPI::Unit unit);
    void onUnitMorph(BWAPI::Unit unit);
};

}
