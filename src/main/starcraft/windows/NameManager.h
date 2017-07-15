#pragma once

#include <BWAPI.h>

namespace ECGBot
{

class NameManager
{
  NameManager();
  static const char*  names[48];
  int                 currentNameIdx;
  std::map<int, std::string>  idToName;
  std::map<std::string, int>  nameToID;

  void        setUnitName(int unitID);
public:

  // singletons
  static NameManager & Instance();

  std::string getUnitName(int unitID);
  int         getUnitID(std::string unitName);

  // TODO: remove unit names on destruction
  void        onUnitCreate(BWAPI::Unit unit);
  void        onUnitShow(BWAPI::Unit unit);

  void        draw();

};

}
