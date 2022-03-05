#include "PresenceHandler.h"
#include "MQTTHandler.h"
#include "pins.h"
#include "Globals.h"

PresenceHandler::PresenceHandler (void)
{
    currentPresenceDetect = digitalRead(PIROutputPin) ? true : false;
    pinMode(PRESENCE_LED_PIN_SINK, OUTPUT);
}

void PresenceHandler::execute(bool newMs)
{
    boolean const currentPIR = digitalRead(PIROutputPin) ? true : false;

    if(currentPIR != currentPresenceDetect)
    {
        currentPresenceDetect = currentPIR;
        digitalWrite(PRESENCE_LED_PIN_SINK, !currentPresenceDetect);
        char format[2];
        format[0] = currentPresenceDetect ? '1' : '0';
        format[1] = '\0';
        MQTTHandler::publisher->publish(MQTTPubPresence, format);
    }
}