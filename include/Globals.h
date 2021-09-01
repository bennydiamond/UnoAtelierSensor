#include <stdint.h>

typedef struct Globals
{
    Globals (void) :
    temperature_tenthC(INT16_MAX),
    humidity_tenthPercent(INT16_MAX),
    geigerPulseCount(0)
    {}
    /* data */
    int16_t temperature_tenthC;
    int16_t humidity_tenthPercent;
    uint16_t geigerPulseCount;
} Globals_t;

extern Globals_t globals;