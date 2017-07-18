#include "ECGUtil.h"
#include <BWAPI.h>

using namespace ECGBot;

UnitDescriptor::UnitDescriptor()
{
  empty = true;
}

UnitDescriptor::UnitDescriptor(int uID, int eIdentifier, int quant, bool aly, BWAPI::UnitType uType, UnitStatus stat, BWAPI::Unitset selected)
{
  empty = false;
  unitID = uID;
  quantity = quant;
  ally = aly;
  ecgIdentifier = eIdentifier;
  unitType = uType;
  status = stat;
  selectedUnits = selected;
  landmark = BWAPI::Position(0, 0);
  region = Region::EXACT;
}

UnitDescriptor::UnitDescriptor(int uID, int eIdentifier, int quant, bool aly, BWAPI::UnitType uType, UnitStatus stat, BWAPI::Unitset selected,
                               BWAPI::Position lmark, Region rgion)
{
  empty = false;
  unitID = uID;
  quantity = quant;
  ally = aly;
  ecgIdentifier = eIdentifier;
  unitType = uType;
  status = stat;
  selectedUnits = selected;
  landmark = lmark;
  region = rgion;
}
