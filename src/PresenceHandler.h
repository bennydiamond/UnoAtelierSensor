#include <stdint.h>

#pragma once

class PresenceHandler
{
public:
    PresenceHandler (void);

    void execute (bool newMs);
private:
    bool currentPresenceDetect;
};