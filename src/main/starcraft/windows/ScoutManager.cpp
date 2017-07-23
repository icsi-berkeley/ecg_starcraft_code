#include "ScoutManager.h"
#include "Global.h"

using namespace UAlbertaBot;

ScoutManager::ScoutManager()
    : _workerScout(nullptr)
    , _numWorkerScouts(0)
    , _scoutUnderAttack(false)
    , _scoutStatus("None")
    , _currentRegionVertexIndex(-1)
    , _previousScoutHP(0)
{
}

void ScoutManager::update()
{
    // calculate enemy region vertices if we haven't yet
    /*if (_enemyRegionVertices.empty())
    {
        calculateEnemyRegionVertices();
    }*/

	moveScouts();
    drawScoutInformation(200, 320);
}

void ScoutManager::setWorkerScout(BWAPI::Unit unit)
{
    // if we have a previous worker scout, release it back to the worker manager
    if (_workerScout)
    {
        Global::Workers().finishedWithWorker(_workerScout);
    }

    _workerScout = unit;
    Global::Workers().setScoutWorker(_workerScout);
}

void ScoutManager::drawScoutInformation(int x, int y)
{
    if (!Config::Debug::DrawScoutInfo)
    {
        return;
    }

    BWAPI::Broodwar->drawTextScreen(x, y, "ScoutInfo: %s", _scoutStatus.c_str());
    for (size_t i(0); i < _enemyRegionVertices.size(); ++i)
    {
        BWAPI::Broodwar->drawCircleMap(_enemyRegionVertices[i], 4, BWAPI::Colors::Green, false);
        BWAPI::Broodwar->drawTextMap(_enemyRegionVertices[i], "%d", i);
    }
}

void ScoutManager::moveScouts()
{
	if (!_workerScout || !_workerScout->exists() || !(_workerScout->getHitPoints() > 0))
	{
		return;
	}

    int scoutHP = _workerScout->getHitPoints() + _workerScout->getShields();

	// get the enemy base location, if we have one
	const BaseLocation * enemyBaseLocation = Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->enemy());

    int scoutDistanceThreshold = 30;

    if (_workerScout->isCarryingGas())
    {
        BWAPI::Broodwar->drawCircleMap(_workerScout->getPosition(), 10, BWAPI::Colors::Purple, true);
    }

	// if we know where the enemy region is and where our scout is
	if (_workerScout && (enemyBaseLocation != nullptr))
	{
        int scoutDistanceToEnemy = Global::Map().getGroundDistance(_workerScout->getPosition(), enemyBaseLocation->getPosition());
        bool scoutInRangeOfenemy = scoutDistanceToEnemy <= scoutDistanceThreshold;

        // we only care if the scout is under attack within the enemy region
        // this ignores if their scout worker attacks it on the way to their base
        if (scoutHP < _previousScoutHP)
        {
	        _scoutUnderAttack = true;
        }

        if (!_workerScout->isUnderAttack() && !enemyWorkerInRadius())
        {
	        _scoutUnderAttack = false;
        }

		// if the scout is in the enemy region
		if (scoutInRangeOfenemy)
		{
			// get the closest enemy worker
			BWAPI::Unit closestWorker = closestEnemyWorker();

			// if the worker scout is not under attack
			if (!_scoutUnderAttack)
			{
				// if there is a worker nearby, harass it
				if (Config::Strategy::ScoutHarassEnemy && closestWorker && (_workerScout->getDistance(closestWorker) < 800))
				{
                    _scoutStatus = "Harass enemy worker";
                    _currentRegionVertexIndex = -1;
					Micro::SmartAttackUnit(_workerScout, closestWorker);
				}
				// otherwise keep moving to the enemy base location
				else
				{
                    _scoutStatus = "Following perimeter";
                    Micro::SmartMove(_workerScout, enemyBaseLocation->getPosition());
                }
			}
			// if the worker scout is under attack
			else
			{
                _scoutStatus = "Under attack inside, fleeing";
                Micro::SmartMove(_workerScout, getFleePosition());
			}
		}
		// if the scout is not in the enemy region
		else if (_scoutUnderAttack)
		{
            _scoutStatus = "Under attack inside, fleeing";

            Micro::SmartMove(_workerScout, getFleePosition());
		}
		else
		{
            _scoutStatus = "Enemy region known, going there";

			// move to the enemy region
			Micro::SmartMove(_workerScout, enemyBaseLocation->getPosition());
        }

	}

	// for each start location in the level
	if (!enemyBaseLocation)
	{
        _scoutStatus = "Enemy base unknown, exploring";

		for (const BaseLocation * startLocation : Global::Bases().getStartingBaseLocations())
		{
			// if we haven't explored it yet
			if (!BWAPI::Broodwar->isExplored(startLocation->getDepotTilePosition()))
			{
				// assign a zergling to go scout it
				Micro::SmartMove(_workerScout, BWAPI::Position(startLocation->getDepotTilePosition()));
				return;
			}
		}
	}

    _previousScoutHP = scoutHP;
}

BWAPI::Unit ScoutManager::closestEnemyWorker()
{
	BWAPI::Unit enemyWorker = nullptr;
	double maxDist = 0;

	BWAPI::Unit geyser = getEnemyGeyser();

	for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
	{
		if (unit->getType().isWorker() && unit->isConstructing())
		{
			return unit;
		}
	}

	// for each enemy worker
	for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
	{
		if (unit->getType().isWorker())
		{
			double dist = unit->getDistance(geyser);

			if (dist < 800 && dist > maxDist)
			{
				maxDist = dist;
				enemyWorker = unit;
			}
		}
	}

	return enemyWorker;
}

BWAPI::Unit ScoutManager::getEnemyGeyser()
{
	BWAPI::Unit geyser = nullptr;
	const BaseLocation * enemyBaseLocation = Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->enemy());

	for (auto & unit : enemyBaseLocation->getGeysers())
	{
		geyser = unit;
	}

	return geyser;
}

bool ScoutManager::enemyWorkerInRadius()
{
	for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
	{
		if (unit->getType().isWorker() && (unit->getDistance(_workerScout) < 300))
		{
			return true;
		}
	}

	return false;
}

bool ScoutManager::immediateThreat()
{
	std::vector<BWAPI::Unit> enemyAttackingWorkers;
	for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
	{
		if (unit->getType().isWorker() && unit->isAttacking())
		{
			enemyAttackingWorkers.push_back(unit);
		}
	}

	if (_workerScout->isUnderAttack())
	{
		return true;
	}

	for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
	{
		double dist = unit->getDistance(_workerScout);
		double range = unit->getType().groundWeapon().maxRange();

		if (unit->getType().canAttack() && !unit->getType().isWorker() && (dist <= range + 32))
		{
			return true;
		}
	}

	return false;
}

int ScoutManager::getClosestVertexIndex(BWAPI::Unit unit)
{
    int closestIndex = -1;
    double closestDistance = 10000000;

    for (size_t i(0); i < _enemyRegionVertices.size(); ++i)
    {
        double dist = unit->getDistance(_enemyRegionVertices[i]);
        if (dist < closestDistance)
        {
            closestDistance = dist;
            closestIndex = i;
        }
    }

    return closestIndex;
}

//void ScoutManager::followPerimeter()
//{
//    BWAPI::Position fleeTo = getFleePosition();
//
//    if (Config::Debug::DrawScoutInfo)
//    {
//        BWAPI::Broodwar->drawCircleMap(fleeTo, 5, BWAPI::Colors::Red, true);
//    }
//
//	Micro::SmartMove(_workerScout, fleeTo);
//}

BWAPI::Position ScoutManager::getFleePosition()
{
    // TODO: make this follow the perimeter of the enemy base again, but for now just use home base as flee direction
    return Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->self())->getPosition();

    //UAB_ASSERT_WARNING(!_enemyRegionVertices.empty(), "We should have an enemy region vertices if we are fleeing");
    //
    //const BaseLocation * enemyBaseLocation = Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->enemy());

    //// if this is the first flee, we will not have a previous perimeter index
    //if (_currentRegionVertexIndex == -1)
    //{
    //    // so return the closest position in the polygon
    //    int closestPolygonIndex = getClosestVertexIndex(_workerScout);

    //    UAB_ASSERT_WARNING(closestPolygonIndex != -1, "Couldn't find a closest vertex");

    //    if (closestPolygonIndex == -1)
    //    {
    //        return BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
    //    }
    //    else
    //    {
    //        // set the current index so we know how to iterate if we are still fleeing later
    //        _currentRegionVertexIndex = closestPolygonIndex;
    //        return _enemyRegionVertices[closestPolygonIndex];
    //    }
    //}
    //// if we are still fleeing from the previous frame, get the next location if we are close enough
    //else
    //{
    //    double distanceFromCurrentVertex = _enemyRegionVertices[_currentRegionVertexIndex].getDistance(_workerScout->getPosition());

    //    // keep going to the next vertex in the perimeter until we get to one we're far enough from to issue another move command
    //    while (distanceFromCurrentVertex < 128)
    //    {
    //        _currentRegionVertexIndex = (_currentRegionVertexIndex + 1) % _enemyRegionVertices.size();

    //        distanceFromCurrentVertex = _enemyRegionVertices[_currentRegionVertexIndex].getDistance(_workerScout->getPosition());
    //    }

    //    return _enemyRegionVertices[_currentRegionVertexIndex];
    //}
}

//void ScoutManager::calculateEnemyRegionVertices()
//{
//    const BaseLocation * enemyBaseLocation = Global::Bases().getPlayerStartingBaseLocation(BWAPI::Broodwar->enemy());
//    //UAB_ASSERT_WARNING(enemyBaseLocation, "We should have an enemy base location if we are fleeing");
//
//    if (!enemyBaseLocation)
//    {
//        return;
//    }
//
//    BWTA::Region * enemyRegion = enemyBaseLocation->getRegion();
//    //UAB_ASSERT_WARNING(enemyRegion, "We should have an enemy region if we are fleeing");
//
//    if (!enemyRegion)
//    {
//        return;
//    }
//
//    const BWAPI::Position basePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
//    const std::vector<BWAPI::TilePosition> & closestTobase = Global::Map().getClosestTilesTo(basePosition);
//
//    std::set<BWAPI::Position> unsortedVertices;
//
//    // check each tile position
//    for (size_t i(0); i < closestTobase.size(); ++i)
//    {
//        const BWAPI::TilePosition & tp = closestTobase[i];
//
//        if (BWTA::getRegion(tp) != enemyRegion)
//        {
//            continue;
//        }
//
//        // a tile is 'surrounded' if
//        // 1) in all 4 directions there's a tile position in the current region
//        // 2) in all 4 directions there's a buildable tile
//        bool surrounded = true;
//        if (BWTA::getRegion(BWAPI::TilePosition(tp.x+1, tp.y)) != enemyRegion || !BWAPI::Broodwar->isBuildable(BWAPI::TilePosition(tp.x+1, tp.y))
//            || BWTA::getRegion(BWAPI::TilePosition(tp.x, tp.y+1)) != enemyRegion || !BWAPI::Broodwar->isBuildable(BWAPI::TilePosition(tp.x, tp.y+1))
//            || BWTA::getRegion(BWAPI::TilePosition(tp.x-1, tp.y)) != enemyRegion || !BWAPI::Broodwar->isBuildable(BWAPI::TilePosition(tp.x-1, tp.y))
//            || BWTA::getRegion(BWAPI::TilePosition(tp.x, tp.y-1)) != enemyRegion || !BWAPI::Broodwar->isBuildable(BWAPI::TilePosition(tp.x, tp.y -1)))
//        {
//            surrounded = false;
//        }
//
//        // push the tiles that aren't surrounded
//        if (!surrounded && BWAPI::Broodwar->isBuildable(tp))
//        {
//            if (Config::Debug::DrawScoutInfo)
//            {
//                int x1 = tp.x * 32 + 2;
//                int y1 = tp.y * 32 + 2;
//                int x2 = (tp.x+1) * 32 - 2;
//                int y2 = (tp.y+1) * 32 - 2;
//
//                BWAPI::Broodwar->drawTextMap(x1+3, y1+2, "%d", Global::Map().getGroundDistance(BWAPI::Position(tp), basePosition));
//                BWAPI::Broodwar->drawBoxMap(x1, y1, x2, y2, BWAPI::Colors::Green, false);
//            }
//
//            unsortedVertices.insert(BWAPI::Position(tp) + BWAPI::Position(16, 16));
//        }
//    }
//
//
//    std::vector<BWAPI::Position> sortedVertices;
//    BWAPI::Position current = *unsortedVertices.begin();
//
//    _enemyRegionVertices.push_back(current);
//    unsortedVertices.erase(current);
//
//    // while we still have unsorted vertices left, find the closest one remaining to current
//    while (!unsortedVertices.empty())
//    {
//        double bestDist = 1000000;
//        BWAPI::Position bestPos;
//
//        for (const BWAPI::Position & pos : unsortedVertices)
//        {
//            double dist = pos.getDistance(current);
//
//            if (dist < bestDist)
//            {
//                bestDist = dist;
//                bestPos = pos;
//            }
//        }
//
//        current = bestPos;
//        sortedVertices.push_back(bestPos);
//        unsortedVertices.erase(bestPos);
//    }
//
//    // let's close loops on a threshold, eliminating death grooves
//    int distanceThreshold = 100;
//
//    while (true)
//    {
//        // find the largest index difference whose distance is less than the threshold
//        int maxFarthest = 0;
//        int maxFarthestStart = 0;
//        int maxFarthestEnd = 0;
//
//        // for each starting vertex
//        for (int i(0); i < (int)sortedVertices.size(); ++i)
//        {
//            int farthest = 0;
//            int farthestIndex = 0;
//
//            // only test half way around because we'll find the other one on the way back
//            for (size_t j(1); j < sortedVertices.size()/2; ++j)
//            {
//                int jindex = (i + j) % sortedVertices.size();
//
//                if (sortedVertices[i].getDistance(sortedVertices[jindex]) < distanceThreshold)
//                {
//                    farthest = j;
//                    farthestIndex = jindex;
//                }
//            }
//
//            if (farthest > maxFarthest)
//            {
//                maxFarthest = farthest;
//                maxFarthestStart = i;
//                maxFarthestEnd = farthestIndex;
//            }
//        }
//
//        // stop when we have no long chains within the threshold
//        if (maxFarthest < 4)
//        {
//            break;
//        }
//
//        double dist = sortedVertices[maxFarthestStart].getDistance(sortedVertices[maxFarthestEnd]);
//
//        std::vector<BWAPI::Position> temp;
//
//        for (size_t s(maxFarthestEnd); s != maxFarthestStart; s = (s+1) % sortedVertices.size())
//        {
//            temp.push_back(sortedVertices[s]);
//        }
//
//        sortedVertices = temp;
//    }
//
//    _enemyRegionVertices = sortedVertices;
//}
