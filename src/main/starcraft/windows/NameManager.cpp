#include "NameManager.h"

using namespace ECGBot;

const char* NameManager::names[48] = {"alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta",
                                      "theta", "iota", "kappa", "lambda", "mu", "nu", "xi", "omicron"
                                      "pi", "rho", "sigma", "tau", "upsilon", "phi", "chi", "psi",
                                      "omega", "bravo", "charlie", "echo", "foxtrot", "golf",
                                      "hotel", "india", "juliett", "kilo", "lima", "mike",
                                      "november", "oscar", "papa", "quebec", "romeo", "sierra",
                                      "tango", "uniform", "victory", "whiskey", "xray",
                                      "yankee", "zulu" };


NameManager::NameManager()
{
  currentName = 0;
}

NameManager & NameManager::Instance()
{
	static NameManager instance;
	return instance;
}

void NameManager::setUnitName(int unitID)
{
  if (namesMap.count(unitID) == 0)
  {
    namesMap[unitID] = currentName;
    currentName++;
  }
}

const char* NameManager::getUnitName(int unitID)
{
  // TODO: This will index out of bounds when we run out of names.
  return names[namesMap[unitID]];
}

void NameManager::onUnitCreate(BWAPI::Unit unit)
{
  if (unit->getPlayer() == BWAPI::Broodwar->self())
    setUnitName(unit->getID());
}

void NameManager::onUnitShow(BWAPI::Unit unit)
{
  if (unit->getPlayer() == BWAPI::Broodwar->self())
    setUnitName(unit->getID());
}

void NameManager::draw()
{
  for (auto &u : BWAPI::Broodwar->self()->getUnits())
    BWAPI::Broodwar->drawTextMap(u->getLeft(), u->getTop(), getUnitName(u->getID()));
}
