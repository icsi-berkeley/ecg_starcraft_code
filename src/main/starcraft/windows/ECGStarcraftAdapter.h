#pragma once
#include <BWAPI.h>
#include "CombatCommander.h"
#include "ProductionManager.h"
#include "Global.h"

// Remember not to use "Broodwar" in any global class constructor!

namespace ECGBot
{

class ECGStarcraftAdapter : public BWAPI::AIModule
{
public:
  // Virtual functions for callbacks, leave these as they are.
  virtual void onStart();
  virtual void onEnd(bool isWinner);
  virtual void onFrame();
  virtual void onSendText(std::string text);
  virtual void onReceiveText(BWAPI::Player player, std::string text);
  virtual void onPlayerLeft(BWAPI::Player player);
  virtual void onNukeDetect(BWAPI::Position target);
  virtual void onUnitDiscover(BWAPI::Unit unit);
  virtual void onUnitEvade(BWAPI::Unit unit);
  virtual void onUnitShow(BWAPI::Unit unit);
  virtual void onUnitHide(BWAPI::Unit unit);
  virtual void onUnitCreate(BWAPI::Unit unit);
  virtual void onUnitDestroy(BWAPI::Unit unit);
  virtual void onUnitMorph(BWAPI::Unit unit);
  virtual void onUnitRenegade(BWAPI::Unit unit);
  virtual void onSaveGame(std::string gameName);
  virtual void onUnitComplete(BWAPI::Unit unit);
  // Everything below this line is safe to modify.
  UAlbertaBot::WorkerManager & Workers();
  UAlbertaBot::ProductionManager & Production();
  const UAlbertaBot::UnitInfoManager & UnitInfo() const;
  const UAlbertaBot::StrategyManager & Strategy() const;
  const UAlbertaBot::BaseLocationManager & Bases() const;
  const UAlbertaBot::MapTools & Map() const;
private:
  UAlbertaBot::WorkerManager       _workerManager;
  UAlbertaBot::UnitInfoManager     _unitInfoManager;
  UAlbertaBot::StrategyManager     _strategyManager;
  UAlbertaBot::MapTools            _mapTools;
  UAlbertaBot::BaseLocationManager _baseLocationManager;
  UAlbertaBot::CombatCommander     _combatCommander;
  UAlbertaBot::ProductionManager   _productionManager;


};

}
