#pragma once

#include <BWAPI.h>
#include <set>

namespace ECGBot
{

class NameManager
{
  NameManager();
  static const char*  names[48];
  int                 currentNameIdx;
  std::map<int, std::string>      idToName;
  std::map<std::string, int>      nameToID;
  std::map<int, int>              uidToEid;
  std::map<int, std::set<int> >   eidToUid;
  std::map<int, int>              producerToEid;

  void        setUnitName(int unitID);
public:

  // singletons
  static NameManager & Instance();

  std::string     getUnitName(int unitID);
  int             getUnitID(std::string unitName);
  int             getECGID(int unitID);
  BWAPI::Unitset  getByECGID(int ecgID);

  void            onUnitReadyFrame(int producerID, int unitID);
  void            onUnitProduction(int ecgID, int producerID);
  // TODO: remove unit names on destruction
  void            onUnitCreate(BWAPI::Unit unit);
  void            onUnitShow(BWAPI::Unit unit);

  void            draw();

};

}
