#include "ECGUtil.h"
#include <BWAPI.h>

using namespace ECGBot;

UnitDescriptor::UnitDescriptor()
{
  empty = true;
  defaultUnits = BWAPI::Broodwar->getSelectedUnits();
}

UnitDescriptor::UnitDescriptor(int uID, int eIdentifier, int quant, Comparator compEnum, bool aly, BWAPI::UnitType uType,
                               UnitStatus stat, BWAPI::Unitset dfault, BWAPI::Position lmark, Region rgion)
{
  empty = false;
  unitID = uID;
  quantity = quant;
  comparator = compEnum;
  ally = aly;
  ecgID = eIdentifier;
  unitType = uType;
  status = stat;
  defaultUnits = dfault;
  landmark = lmark;
  region = rgion;
}
