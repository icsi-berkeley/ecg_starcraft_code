#include "Global.h"
#include "ECGStarcraftAdapter.h"

using namespace UAlbertaBot;

ECGBot::ECGStarcraftAdapter * uabModule = nullptr;

void Global::SetModule(ECGBot::ECGStarcraftAdapter * module)
{
    uabModule = module;
}

const StrategyManager & Global::Strategy()
{
    return uabModule->Strategy();
}

const BaseLocationManager & Global::Bases()
{
    return uabModule->Bases();
}

WorkerManager & Global::Workers()
{
    return uabModule->Workers();
}

const UnitInfoManager & Global::UnitInfo()
{
    return uabModule->UnitInfo();
}

const MapTools & Global::Map()
{
    return uabModule->Map();
}

ProductionManager & Global::Production()
{
    return uabModule->Production();
}
