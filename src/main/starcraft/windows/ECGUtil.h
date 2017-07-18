#pragma once

#include <BWAPI.h>
#include "rapidjson/document.h"

namespace ECGBot
{

enum Resource {minerals, gas, supply};
enum Comparator {greater, less, equal};
enum EventKind {ONCE, WHILE, UNTIL, IF};
enum UnitStatus {IDLE, ATTACKING, DEFENDING, GATHERING, EXPLORING, HARASSING, BUILDING, NA};
enum Region {EXACT, CLOSE, DISTANT, RIGHT, LEFT, BACK, FRONT};

struct UnitDescriptor
{
  bool empty;
	bool ally;
  int unitID; // <= 0 means not set
	int ecgIdentifier; // <= 0 means not set
	int quantity; // <= 0 means not set
  BWAPI::UnitType unitType; // BWAPI::UnitTypes::AllUnits is default
  UnitStatus status; // NA means not set
	BWAPI::Position landmark; // 0, 0 means not set
	Region region; // Default to EXACT, NOT used if landmark is not set (i.e. 0, 0),
	BWAPI::Unitset selectedUnits;

  UnitDescriptor();
	UnitDescriptor(int uID, int uIdentifier, int quant, bool aly, BWAPI::UnitType uType, UnitStatus stat, BWAPI::Unitset selected);
  UnitDescriptor(int uID, int uIdentifier, int quant, bool aly, BWAPI::UnitType uType, UnitStatus stat, BWAPI::Unitset selected,
								 BWAPI::Position lmark, Region rgion);
};

class ECGUtil
{

public:
	static BWAPI::UnitType ECGUtil::getUnitType(const char* unitName) {
		if (strcmp(unitName, "commandcenter") == 0) {
			return BWAPI::UnitTypes::Terran_Command_Center;
		}
		else if (strcmp(unitName, "barracks") == 0) {
			return BWAPI::UnitTypes::Terran_Barracks;
		}
		else if (strcmp(unitName, "refinery") == 0) {
			return BWAPI::UnitTypes::Terran_Refinery;
		}
		else if (strcmp(unitName, "marine") == 0) {
			return BWAPI::UnitTypes::Terran_Marine;
		}
		else if (strcmp(unitName, "scv") == 0) {
			return BWAPI::UnitTypes::Terran_SCV;
		}
		else if (strcmp(unitName, "minerals") == 0) {
			return BWAPI::UnitTypes::Resource_Mineral_Field;
		}
		else if (strcmp(unitName, "gas") == 0) {
			return BWAPI::UnitTypes::Resource_Vespene_Geyser;
		}
		return BWAPI::UnitTypes::AllUnits;
	}

	static std::string ECGUtil::getUnitName(BWAPI::UnitType unitType) {
		if (unitType == BWAPI::UnitTypes::Terran_Command_Center) {
			return "commandcenter";
		}
		else if (unitType == BWAPI::UnitTypes::Terran_Barracks) {
			return "barracks";
		}
		else if (unitType == BWAPI::UnitTypes::Terran_Refinery) {
			return "refinery";
		}
		else if (unitType == BWAPI::UnitTypes::Terran_Marine) {
			return "marine";
		}
		else if (unitType == BWAPI::UnitTypes::Terran_SCV) {
			return "scv";
		}
		else if (unitType == BWAPI::UnitTypes::Resource_Mineral_Field) {
			return "mineral";
		}
		else if (unitType == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
			return "gas";
		}
		return "unknown";
	}

	static BWAPI::Unitset ECGUtil::resolveUnitDescriptor(UnitDescriptor ud)
	{
		BWAPI::Unitset matchedSet = BWAPI::Unitset();
		if (ud.empty)
			return matchedSet;

		if (ud.unitID > 0)
		{
			matchedSet.insert(BWAPI::Broodwar->getUnit(ud.unitID));
			return matchedSet;
		}

		// TODO: Do something with the ECGidentifier and quantity

		if (ud.landmark.x > 0 || ud.landmark.y > 0)
		{
			// TODO: filter on unit type within a region and ally
			// BWAPI::Broodwar->getUnitsInRadius(landmark, ...)
		}
		else
		{
			// filter only on unit type and ally
			if (ud.ally)
				matchedSet = BWAPI::Broodwar->getUnitsInRadius(0, 0, 999999,
					BWAPI::Filter::GetType == ud.unitType && BWAPI::Filter::IsAlly);
			else
				matchedSet = BWAPI::Broodwar->getUnitsInRadius(0, 0, 999999,
					BWAPI::Filter::GetType == ud.unitType && BWAPI::Filter::IsEnemy);
		}

		if (ud.status != UnitStatus::NA)
		{
			//TODO: Add cases for other statuses
			switch (ud.status) {
				case UnitStatus::IDLE:
					BWAPI::Unitset filteredSet = BWAPI::Unitset();
					for (auto & unit : matchedSet)
					{
						if (unit->isIdle())
							filteredSet.insert(unit);
					}
					matchedSet = filteredSet;
			}
		}

		if (!ud.selectedUnits.empty())
		{

			BWAPI::Unitset intersection;
			for (auto & worker : ud.selectedUnits)
		  {
		    if (matchedSet.find(worker) != matchedSet.end())
					intersection.insert(worker);
		  }

			if (!intersection.empty())
				return intersection;
		}

		return matchedSet;
	}

};

}
