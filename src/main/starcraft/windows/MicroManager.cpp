#include "MicroManager.h"
#include "Global.h"

using namespace UAlbertaBot;

MicroManager::MicroManager()
{
}

void MicroManager::setUnits(const std::vector<BWAPI::Unit> & u)
{
	_units = u;
}

void MicroManager::execute(const SquadOrder & inputOrder)
{
	// Nothing to do if we have no units
	if (_units.empty() || !(inputOrder.getType() == SquadOrderTypes::Attack || inputOrder.getType() == SquadOrderTypes::Defend))
	{
		return;
	}

	order = inputOrder;
	drawOrderText();

	// Discover enemies within region of interest
	std::vector<BWAPI::Unit> nearbyEnemies;

	// if the order is to defend, we only care about units in the radius of the defense
	if (order.getType() == SquadOrderTypes::Defend)
	{
		Global::Map().GetUnitsInRadius(nearbyEnemies, order.getPosition(), order.getRadius(), false, true);

	} // otherwise we want to see everything on the way
	else if (order.getType() == SquadOrderTypes::Attack)
	{
		Global::Map().GetUnitsInRadius(nearbyEnemies, order.getPosition(), order.getRadius(), false, true);
		for (auto & unit : _units)
		{
			BWAPI::Unit u = unit;
			BWAPI::UnitType t = u->getType();
			Global::Map().GetUnitsInRadius(nearbyEnemies, unit->getPosition(), order.getRadius(), false, true);
		}
	}

	// the following block of code attacks all units on the way to the order position
	// we want to do this if the order is attack, defend, or harass
	if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
	{
        // if this is a worker defense force
        if (_units.size() == 1 && (*_units.begin())->getType().isWorker())
        {
            executeMicro(nearbyEnemies);
        }
        // otherwise it is a normal attack force
        else
        {
            // if this is a defense squad then we care about all units in the area
            if (order.getType() == SquadOrderTypes::Defend)
            {
                executeMicro(nearbyEnemies);
            }
            // otherwise we only care about workers if they are in their own region
            else
            {
                 // if this is the an attack squad
                std::vector<BWAPI::Unit> workersRemoved;

                for (auto & enemyUnit : nearbyEnemies)
		        {
                    // if its not a worker add it to the targets
			        if (!enemyUnit->getType().isWorker())
                    {
                        workersRemoved.push_back(enemyUnit);
                    }
                    // if it is a worker
                    else
                    {
                        for (const BaseLocation * enemyBaseLocation : Global::Bases().getOccupiedBaseLocations(BWAPI::Broodwar->enemy()))
                        {
                            // only add it if it's in their region
                            if (enemyBaseLocation->containsPosition(enemyUnit->getPosition()))
                            {
                                workersRemoved.push_back(enemyUnit);
                            }
                        }
                    }
		        }

		        // Allow micromanager to handle enemies
		        executeMicro(workersRemoved);
            }
        }
	}
}

const std::vector<BWAPI::Unit> & MicroManager::getUnits() const
{
    return _units;
}

void MicroManager::regroup(const BWAPI::Position & regroupPosition) const
{
    BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
    int regroupDistanceFromBase = Global::Map().getGroundDistance(regroupPosition, ourBasePosition);

	// for each of the units we have
	for (auto & unit : _units)
	{
        int unitDistanceFromBase = Global::Map().getGroundDistance(unit->getPosition(), ourBasePosition);

		// if the unit is outside the regroup area
        if (unitDistanceFromBase > regroupDistanceFromBase)
        {
            Micro::SmartMove(unit, ourBasePosition);
        }
		else if (unit->getDistance(regroupPosition) > 100)
		{
			// regroup it
			Micro::SmartMove(unit, regroupPosition);
		}
		else
		{
			Micro::SmartAttackMove(unit, unit->getPosition());
		}
	}
}

// returns true if position:
// a) is walkable
// b) doesn't have buildings on it
// c) doesn't have a unit on it that can attack ground
bool MicroManager::checkPositionWalkable(BWAPI::Position pos)
{
	// get x and y from the position
	int x(pos.x), y(pos.y);

	// walkable tiles exist every 8 pixels
	bool good = BWAPI::Broodwar->isWalkable(x/8, y/8);

	// if it's not walkable throw it out
	if (!good) return false;

	// for each of those units, if it's a building or an attacking enemy unit we don't want to go there
	for (auto & unit : BWAPI::Broodwar->getUnitsOnTile(x/32, y/32))
	{
		if	(unit->getType().isBuilding() || unit->getType().isResourceContainer() ||
			(unit->getPlayer() != BWAPI::Broodwar->self() && unit->getType().groundWeapon() != BWAPI::WeaponTypes::None))
		{
				return false;
		}
	}

	// otherwise it's okay
	return true;
}

void MicroManager::trainSubUnits(BWAPI::Unit unit) const
{
	if (unit->getType() == BWAPI::UnitTypes::Protoss_Reaver)
	{
		unit->train(BWAPI::UnitTypes::Protoss_Scarab);
	}
	else if (unit->getType() == BWAPI::UnitTypes::Protoss_Carrier)
	{
		unit->train(BWAPI::UnitTypes::Protoss_Interceptor);
	}
}

void MicroManager::drawOrderText()
{
	for (auto & unit : _units)
    {
		if (Config::Debug::DrawUnitTargetInfo) BWAPI::Broodwar->drawTextMap(unit->getPosition().x, unit->getPosition().y, "%s", order.getStatus().c_str());
	}
}
