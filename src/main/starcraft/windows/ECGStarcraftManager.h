#pragma once
#include "BWAPI.h"
#include "ECGUtil.h"

namespace ECGBot
{

class ECGStarcraftManager
{
    ECGStarcraftManager();

public:

    // singletons
    static ECGStarcraftManager & Instance();

    void    build(UnitDescriptor commandedUnits, const BWAPI::UnitType unitType, int count);
    void    gather(UnitDescriptor commandedUnits, const BWAPI::UnitType resourceType);

};
}
