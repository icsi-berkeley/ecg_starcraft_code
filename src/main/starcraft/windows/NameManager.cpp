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
  currentNameIdx = 0;
}

NameManager & NameManager::Instance()
{
	static NameManager instance;
	return instance;
}

void NameManager::setUnitName(int unitID)
{
  if (idToName.count(unitID) == 0)
  {
    // TODO: This will index out of bounds when we run out of names.
    idToName[unitID] = names[currentNameIdx];
    nameToID[names[currentNameIdx]] = unitID;
    currentNameIdx++;
  }
}

std::string NameManager::getUnitName(int unitID)
{
  return idToName[unitID];
}

int NameManager::getUnitID(std::string unitName)
{
  return nameToID[unitName];
}

int NameManager::getECGID(int unitID)
{
  return uidToEid[unitID];
}

void NameManager::onUnitReadyFrame(int producerID, int unitID)
{
  int ecgID = producerToEid[producerID];
  if (ecgID > 0 && uidToEid.count(unitID) == 0)
  {
    uidToEid[unitID] = ecgID;
    eidToUid[ecgID].insert(unitID);
  }
}

void NameManager::onUnitProduction(int ecgID, int producerID)
{
  producerToEid[producerID] = ecgID;
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
    BWAPI::Broodwar->drawTextMap(u->getLeft(), u->getTop(), getUnitName(u->getID()).c_str());
}
