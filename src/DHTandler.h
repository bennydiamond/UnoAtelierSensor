#include "ExecTimer.h"
#include <dht.h>
#include <stdint.h>

#pragma once

// DHT sensor properties
#define DHTInvalidValue 'N'
#define TempReadLoopPeriod_ms         (60000)
#define DHTStringLen (7)
#define DHTHumHyst_tenth  (5)
#define DHTTempHyst_tenth (2)

class DHTHandler : private ExecTimer
{
public:
    DHTHandler (void);

    void execute (bool newMs);
private:
    dht DHT;
};