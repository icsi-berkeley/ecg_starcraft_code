#pragma once
#include "BWAPI.h"
#include "ECGUtil.h"
#include "Message.h"

namespace ECGBot
{

class ECGStarcraftManager
{
    ECGStarcraftManager();

public:

    // singletons
    static ECGStarcraftManager & Instance();

    void    evaluateAction(Message* message);

    void    build(Message* message);
    void    gather(Message* message);
    void    move(Message* message);

};
}
