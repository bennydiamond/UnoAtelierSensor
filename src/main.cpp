#include <Arduino.h>
#include "Globals.h"
#include "pins.h"
#include "DHTandler.h"
#include "MQTTHandler.h"
#include "PresenceHandler.h"

#define VERSION "1.0"

/* Pin in use
   D10, D11, D12, D13 Ethernet adapter (SPI)
   D2, D3 PIR (VCC, output)
   D7, D8, D9 DHT (GND, VCC, output)
*/

#define UARTBAUD                    (115200)

// Class definitions
Globals_t globals;
MQTTHandler mqtt;
DHTHandler dhtHandler;
PresenceHandler presenceHandler;

// Static variables
static unsigned long previous;

void setup() 
{
  pinMode(DHTGNDPin, OUTPUT);
  pinMode(DHTVCCPin, OUTPUT);
  digitalWrite(DHTGNDPin, LOW);
  digitalWrite(DHTVCCPin, HIGH);

  pinMode(PIRVCCPin, OUTPUT);
  digitalWrite(PIRVCCPin, HIGH);

  Serial.begin(UARTBAUD);

  Serial.print(F("Atelier Multisensor MQTT interface "));
  Serial.println(F(VERSION));

  // Initializes ethernet with DHCP
  mqtt.init();

  previous = 0;
  Serial.println(F("Setup Complete."));
  delay(500);
}

void loop() 
{
  unsigned long const current = millis();
  bool const newMs = previous != current;

  if(newMs)
  {
    previous = current;
  }

  if(mqtt.execute(newMs))
  {
    presenceHandler.execute(newMs);
    dhtHandler.execute(newMs);
  }
}
