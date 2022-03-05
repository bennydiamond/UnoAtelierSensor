#include "ExecTimer.h"
#include <stdint.h>

#pragma once

class PresenceHandler : private ExecTimer
{
public:
    PresenceHandler (void);

    void execute (bool newMs);
private:
    boolean currentPresenceDetect;
};