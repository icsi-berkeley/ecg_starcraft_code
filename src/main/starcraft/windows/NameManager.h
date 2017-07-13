#pragma once

#include <BWAPI.h>

namespace ECGBot
{

class NameManager
{
  NameManager();
  static const char*  names[48];
  int                 currentName;
  std::map<int, int>  namesMap;

  void        setUnitName(int unitID);
public:

  // singletons
  static NameManager & Instance();

  const char* getUnitName(int unitID);

  // TODO: remove unit names on destruction
  void        onUnitCreate(BWAPI::Unit unit);
  void        onUnitShow(BWAPI::Unit unit);

  void        draw();

};

}
