#include "DHTandler.h"
#include "MQTTHandler.h"
#include "pins.h"
#include "Globals.h"

DHTHandler::DHTHandler (void) :
ExecTimer::ExecTimer(TempReadLoopPeriod_ms)
{
    DHT.setDisableIRQ(false);
}

void DHTHandler::execute(bool newMs)
{
    if(newMillisecond(newMs))
    {
        setPeriod(TempReadLoopPeriod_ms);
        DHT.read(DHTPIN);
        float const temp = DHT.temperature;
        float const hum = DHT.humidity;

        int16_t const trimTemp = (temp * 10.0);
        bool const tempHyst = abs(trimTemp - globals.temperature_tenthC) >= DHTTempHyst_tenth;
        if(tempHyst)
        {
            char format[DHTStringLen + 1];
            // Minus len to left align
            dtostrf(temp, -DHTStringLen, 2, format);
            for(uint8_t i = 0; i < DHTStringLen; i++)
            {
                if(format[i] == ' ')
                {
                format[i] = '\0';
                break;
                }
            }
            if(DHTInvalidValue != format[0])
            {
                if(MQTTHandler::publisher->publish(MQTTPubTemperature, format, MQTTRetain))
                {
                    globals.temperature_tenthC = trimTemp;
                }
            }
        }

        int16_t const humTrim = (hum * 10.0);
        bool const humHyst = abs(humTrim - globals.humidity_tenthPercent) >= DHTHumHyst_tenth;
        if(humHyst)
        {
            char format[DHTStringLen + 1];
            dtostrf(hum, -DHTStringLen, 2, format);
            for(uint8_t i = 0; i < DHTStringLen; i++)
            {
                if(format[i] == ' ')
                {
                format[i] = '\0';
                break;
                }
            }
            if(DHTInvalidValue != format[0])
            {
                if(MQTTHandler::publisher->publish(MQTTPubHumidity, format, MQTTRetain))
                {
                    globals.humidity_tenthPercent = humTrim;
                }
            }
        }
    }
}