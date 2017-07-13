#pragma once
#include "BWAPI.h"
#include "MessageManager.h"

namespace ECGBot
{

class ECGStarcraftManager
{
    ECGStarcraftManager();

public:

    // singletons
    static ECGStarcraftManager & Instance();

    void    build(Units commandedUnit, const BWAPI::UnitType unitType, int count);
    void    gather(Units commandedUnit, const BWAPI::UnitType resourceType);

};
}
