#include <stdint.h>

#pragma once

class ExecTimer
{
public:
    ExecTimer (uint16_t const setPeriod_ms)
    {
        setPeriod(setPeriod_ms);
    }

    bool newMillisecond (bool newMs)
    {
        if(newMs)
        {
            if(countdown_ms)
            {
                countdown_ms--;
            }
        }

        return countdown_ms == 0;
    }

    void setPeriod (uint16_t newPeriod)
    {
        countdown_ms = newPeriod;
    }
private:
    uint16_t countdown_ms;
};