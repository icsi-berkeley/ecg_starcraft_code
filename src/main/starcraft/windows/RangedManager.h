#pragma once;

#include "Common.h"
#include "MicroManager.h"

namespace UAlbertaBot
{
class RangedManager : public MicroManager
{
public:

	RangedManager();
	void executeMicro(const std::vector<BWAPI::Unit> & targets);

	int getAttackPriority(BWAPI::Unit rangedUnit, BWAPI::Unit target);
	BWAPI::Unit getTarget(BWAPI::Unit rangedUnit, const std::vector<BWAPI::Unit> & targets);

    void assignTargets(const std::vector<BWAPI::Unit> & targets);
};
}
