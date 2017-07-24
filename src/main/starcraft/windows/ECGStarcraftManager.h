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

    // TODO: there is no reason not to make all these functions static
    void    evaluateAction(Message* message, bool* blocking = nullptr);

    void    build(Message* message, bool* blocking);
    void    gather(Message* message, bool* blocking);
    void    move(Message* message, bool* blocking);

};
}
