#pragma once

#include <BWAPI.h>
#include "rapidjson/document.h"

namespace ECGBot
{

enum Resource {MINERALS, GAS, SUPPLY};
enum Comparator {GEQ, LEQ, EQ};
enum EventKind {ONCE, WHILE, UNTIL, ALWAYS};
enum UnitStatus {IDLE, GATHERING, BUILDING, UNDERATTACK, ATTACKING, DEFENDING, NA};
enum Region {EXACT, CLOSE, DISTANT, RIGHT, LEFT, BACK, FRONT};

struct UnitDescriptor
{
  bool empty;
	bool ally;
  int unitID; // <= 0 means not set TODO: may need to change this to accomodate squad name
	int ecgID; // <= 0 means not set
	int quantity; // <= 0 means not set
  Comparator comparator;
  BWAPI::UnitType unitType; // BWAPI::UnitTypes::AllUnits is default
  UnitStatus status; // NA means not set
	BWAPI::Position landmark; // BWAPI::Positions::None means not set
	Region region; // Default to EXACT, NOT used if landmark is not set (i.e. None),
	BWAPI::Unitset defaultUnits;

  UnitDescriptor();
  UnitDescriptor(int uID, int uIdentifier, int quant, Comparator compEnum, bool aly, BWAPI::UnitType uType,
								 UnitStatus stat, BWAPI::Unitset dfault, BWAPI::Position lmark, Region rgion);
};

class ECGUtil
{

public:
	static BWAPI::UnitType ECGUtil::getUnitType(const char* unitName) {
    if (unitName == NULL) {
      return BWAPI::UnitTypes::AllUnits;
    }
    else if (strcmp(unitName, "commandcenter") == 0) {
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

  static BWAPI::Unitset ECGUtil::resolveUnitDescriptor(UnitDescriptor ud, const BWAPI::UnitFilter extraFilter = BWAPI::Filter::IsCompleted)
	{
		if (ud.empty)
			return ud.defaultUnits;

    BWAPI::Unitset matchedSet;
    // Use Identifiers
		if (ud.unitID > 0)
			matchedSet.insert(BWAPI::Broodwar->getUnit(ud.unitID));
		//else if (ud.ecgID > 0)
		  // TODO: Do something with the ECGidentifier

    // Filter based on type and alliance
    BWAPI::Unitset filteredSet = BWAPI::Unitset();
		const BWAPI::UnitFilter allyFilter = ud.ally ? BWAPI::Filter::IsAlly : BWAPI::Filter::IsEnemy;
    if (ud.unitType == BWAPI::UnitTypes::AllUnits)
      filteredSet = BWAPI::Broodwar->getUnitsInRadius(0, 0, 999999, allyFilter && extraFilter);
    else
      filteredSet = BWAPI::Broodwar->getUnitsInRadius(0, 0, 999999, BWAPI::Filter::GetType == ud.unitType && allyFilter && extraFilter);

    // Filter based on status
    //TODO: Add cases for squads
		switch (ud.status) {
			case UnitStatus::IDLE:
				for (auto & unit : filteredSet)
					if (unit->isIdle())
						matchedSet.insert(unit);
        break;
      case UnitStatus::GATHERING:
        for (auto & unit : filteredSet)
          if (unit->isGatheringGas() || unit->isGatheringMinerals())
            matchedSet.insert(unit);
        break;
      case UnitStatus::BUILDING:
        for (auto & unit : filteredSet)
          if (unit->isConstructing())
            matchedSet.insert(unit);
        break;
      case UnitStatus::UNDERATTACK:
        for (auto & unit : filteredSet)
          if (unit->isUnderAttack())
            matchedSet.insert(unit);
        break;
      case UnitStatus::ATTACKING:
        for (auto & unit : filteredSet)
          if (unit->isAttacking())
            matchedSet.insert(unit);
        break;
      case UnitStatus::DEFENDING:
        for (auto & unit : filteredSet)
          // if (unit->isDefending()) // TODO: use squads to make this check meaningful
          matchedSet.insert(unit);
        break;
      case UnitStatus::NA:
				matchedSet = filteredSet;
		}

    // Filter based on directional region
    BWAPI::Position matchedSetPosition = matchedSet.getPosition();
    if (ud.landmark != BWAPI::Positions::None)
    {
      BWAPI::Unitset locatedSet;
      switch(ud.region)
      {
        case Region::RIGHT:
          for (auto & unit : matchedSet)
            if (matchedSetPosition.x > ud.landmark.x)
              locatedSet.insert(unit);
          break;
        case Region::LEFT:
          for (auto & unit : matchedSet)
            if (matchedSetPosition.x < ud.landmark.x)
              locatedSet.insert(unit);
          break;
        case Region::BACK:
          for (auto & unit : matchedSet)
            if (matchedSetPosition.y > ud.landmark.y)
              locatedSet.insert(unit);
          break;
        case Region::FRONT:
          for (auto & unit : matchedSet)
            if (matchedSetPosition.y < ud.landmark.y)
              locatedSet.insert(unit);
          break;
        default:
          locatedSet = matchedSet;
      }
      matchedSet = locatedSet;
    }

    // Filter based on quantity and proximity region
    if (ud.comparator == Comparator::EQ && (int) matchedSet.size() > ud.quantity)
    {
      if (ud.landmark == BWAPI::Positions::None)
        return BWAPI::Unitset(); // Equivalent of saying select the 5 x but there are actually 6 of x
      else
      {
        BWAPI::Unitset comparedSet;
        std::vector<std::pair<int, BWAPI::Unit>> vals;

        if (ud.region == Region::DISTANT)
          for (auto & unit : matchedSet)
            vals.push_back(std::make_pair(-1 * matchedSetPosition.getDistance(ud.landmark), unit));
        else
          for (auto & unit : matchedSet)
            vals.push_back(std::make_pair(matchedSetPosition.getDistance(ud.landmark), unit));

        std::nth_element(vals.begin(), vals.begin() + ud.quantity, vals.end());
        for (int i = 0; i < ud.quantity; i++)
          comparedSet.insert(vals[i].second);

        matchedSet = comparedSet;
      }
    }
    else if (ud.comparator == Comparator::GEQ && (int) matchedSet.size() < ud.quantity)
      return BWAPI::Unitset();
    else if (ud.comparator == Comparator::LEQ && (int) matchedSet.size() > ud.quantity)
      return BWAPI::Unitset();


    if (!ud.defaultUnits.empty())
		{
			BWAPI::Unitset intersection;
			for (auto & unit : ud.defaultUnits)
		    if (matchedSet.find(unit) != matchedSet.end())
					intersection.insert(unit);

			if (!intersection.empty() && !(ud.comparator == Comparator::GEQ && (int) intersection.size() < ud.quantity))
				return intersection;
		}

		return matchedSet;
	}

  static BWAPI::Position ECGUtil::getMousePosition() {
    return (BWAPI::Broodwar->getScreenPosition() + BWAPI::Broodwar->getMousePosition()).makeValid();
  }

};

}
