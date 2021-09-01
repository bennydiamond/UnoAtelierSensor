#include "ExecTimer.h"
#include "NewPing.h"
#include "RunningMedian.h"
#include <stdint.h>

#pragma once

// Sonar (ping) properties
#define MAX_DISTANCE 120 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
#define MIN_DISTANCE 2 // Minimum distance ping sensor can realisticly measure
#define MEDIAN_SAMPLE_COUNT (13)
#define PING_DIFF_VALUE_THRESHOLD (20)
#define SonarPingLoopPeriod_ms        (40)
#define SonarPingSleepPeriod_ms       (5000)
#define SonarPingPresenceTimeout_ms   (10000)
//#define PING_DEBUG_PRINT

class PresenceHandler : private ExecTimer
{
public:
    PresenceHandler (void);

    void execute (bool newMs);
private:
    NewPing sonar;
    RunningMedian<uint8_t> samples;

    boolean currentPresenceDetect;
    uint8_t previousPresenceValue;
    boolean runPresenceDetectTimeout;
    uint32_t presenceDetectTimer;
};