#pragma once

#include "StrategyManager.h"
#include "WorkerManager.h"
#include "UnitInfoManager.h"
#include "MapTools.h"
#include "BaseLocationManager.h"
#include "ProductionManager.h"

namespace ECGBot
{
  class ECGStarcraftAdapter;
}

namespace UAlbertaBot
{

namespace Global
{
    void SetModule(ECGBot::ECGStarcraftAdapter * module);

    const BaseLocationManager & Bases();
    const StrategyManager & Strategy();
    WorkerManager & Workers();
    const UnitInfoManager & UnitInfo();
    const MapTools & Map();
    ProductionManager & Production();
};
}
