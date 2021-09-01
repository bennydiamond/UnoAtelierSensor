#include "PresenceHandler.h"
#include "MQTTHandler.h"
#include "pins.h"
#include "Globals.h"

PresenceHandler::PresenceHandler (void) :
ExecTimer::ExecTimer(SonarPingLoopPeriod_ms),
sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE),
samples(MEDIAN_SAMPLE_COUNT),
currentPresenceDetect(true),
previousPresenceValue(MAX_DISTANCE),
runPresenceDetectTimeout(true),
presenceDetectTimer(0)
{
    pinMode(PRESENCE_LED_PIN_SINK, OUTPUT);
    digitalWrite(PRESENCE_LED_PIN_SINK, !currentPresenceDetect);
}

void PresenceHandler::execute(bool newMs)
{
    if(newMillisecond(newMs))
    {
        setPeriod(SonarPingLoopPeriod_ms);
        //noInterrupts();
        unsigned int const ping = sonar.ping();
        //interrupts();
        uint8_t const distance = sonar.convert_cm(ping);
        samples.add(distance);

        bool presenceChanged = false;
        uint8_t pingValue = previousPresenceValue;

        if(samples.isFull())
        {
            // TODO remove return to ping value 0 on Presence timeout.
            uint8_t const medianValue = samples.getMedian();
            uint8_t const differentValue = abs(previousPresenceValue - medianValue) > PING_DIFF_VALUE_THRESHOLD;
            if(false == runPresenceDetectTimeout) // Normal behavior
            {
#ifdef PING_DEBUG_PRINT
                Serial.println("normal");
#endif
                pingValue = medianValue;
                previousPresenceValue = medianValue;
                runPresenceDetectTimeout = true;
            }
            else if(differentValue) // Stuck at same value for too long but it has now changed.
            {
#ifdef PING_DEBUG_PRINT
                Serial.println("Diff value");
#endif
                pingValue = previousPresenceValue = medianValue;
                presenceDetectTimer = 0;
                runPresenceDetectTimeout = true;
            }
        }
        else
        {
            pingValue = 0;
        }
#ifdef PING_DEBUG_PRINT
        Serial.println(pingValue);
#endif

        if((true == runPresenceDetectTimeout) && (presenceDetectTimer >= SonarPingPresenceTimeout_ms))
        {
            if(currentPresenceDetect)
            {
#ifdef PING_DEBUG_PRINT
                Serial.println("timer bust");
#endif
                presenceChanged = true;
            }
        }
        else
        {
            if(currentPresenceDetect)
            {
                if(pingValue < MIN_DISTANCE)
                {
                    presenceChanged = true;
                    setPeriod(SonarPingSleepPeriod_ms); // To not toggle presence state to many times in a short time.
                    presenceDetectTimer = 0;
                    runPresenceDetectTimeout = false;
#ifdef PING_DEBUG_PRINT
                    Serial.println("reset");
#endif
                }
            }
            else
            {
                if(pingValue >= MIN_DISTANCE)
                {
                    presenceChanged = true;
#ifdef PING_DEBUG_PRINT
                    Serial.println("trigger");
#endif
                }
            }
        }

        if(presenceChanged)
        {
            currentPresenceDetect = !currentPresenceDetect;
            digitalWrite(PRESENCE_LED_PIN_SINK, !currentPresenceDetect);
            char format[2];
            format[0] = currentPresenceDetect ? '1' : '0';
            format[1] = '\0';
            MQTTHandler::publisher->publish(MQTTPubPresence, format);
        }
    }
}