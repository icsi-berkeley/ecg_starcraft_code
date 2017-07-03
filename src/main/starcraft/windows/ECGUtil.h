#pragma once

#include <BWAPI.h>

namespace ECGBot
{

class ECGUtil
{

public:
	static BWAPI::UnitType ECGUtil::getUnitType(const std::string unitName) {
		if (!unitName.compare("commandcenter")) {
			return BWAPI::UnitTypes::Terran_Command_Center;
		}
		else if (!unitName.compare("barracks")) {
			return BWAPI::UnitTypes::Terran_Barracks;
		}
		else if (!unitName.compare("refinery")) {
			return BWAPI::UnitTypes::Terran_Refinery;
		}
		else if (!unitName.compare("marine")) {
			return BWAPI::UnitTypes::Terran_Marine;
		}
		else if (!unitName.compare("scv")) {
			return BWAPI::UnitTypes::Terran_SCV;
		}
		else if (!unitName.compare("mineral")) {
			return BWAPI::UnitTypes::Resource_Mineral_Field;
		}
		else if (!unitName.compare("gas")) {
			return BWAPI::UnitTypes::Resource_Vespene_Geyser;
		}
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

};

}
