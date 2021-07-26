#include <Arduino.h>
#include "UIPEthernet.h"
#include "PubSubClient.h"
#include "dht.h"
#include "NewPing.h"
#include "RunningMedian.h"
#include "secret.h"

#define VERSION "1.0"

/* Pin in use
   D10, D11, D12, D13 Ethernet adapter (SPI)
   D21, D20 Sonar (trig, echo)
   D7, D8, D9 DHT (GND, VCC, output)
*/

// Sonar (ping) properties
#define TRIGGER_PIN  19  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     18  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 120 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
#define MIN_DISTANCE 2 // Minimum distance ping sensor can realisticly measure
#define MEDIAN_SAMPLE_COUNT (13)
#define PING_DIFF_VALUE_THRESHOLD (20)
#define SonarPingLoopPeriod_ms        (40)
#define SonarPingSleepPeriod_ms       (5000)
#define SonarPingPresenceTimeout_ms   (10000)
//#define PING_DEBUG_PRINT

#define PRESENCE_LED_PIN_SINK (14)

#define UARTBAUD                    (115200)

// MQTT Properties
IPAddress const MQTTBrokerIP        (192, 168, 1, 1);
#define MQTTBrokerPort              (1883)
#define MQTTBrokerUser              SecretMQTTUsername // Username this device should connect with. Define string in secret.h
#define MQTTBrokerPass              SecretMQTTPassword // Password this device should connect with. Define string in secret.h
#define MQTTClientName              "ateliermultisensor"
#define MQTTTopicPrefix             MQTTClientName
#define MQTTTopicGet                "/get"
#define MQTTTopicSet                "/set"
#define MQTTSubscribeTopic          MQTTTopicPrefix MQTTTopicSet  
#define MQTTPubAvailable            MQTTTopicPrefix MQTTTopicGet "/available"
#define MQTTPubPresence             MQTTTopicPrefix MQTTTopicGet "/presence"
#define MQTTPubTemperature          MQTTTopicPrefix MQTTTopicGet "/temperature"
#define MQTTPubHumidity             MQTTTopicPrefix MQTTTopicGet "/humidity"
#define MQTTNotRetain               (false)
#define MQTTRetain                  (true)
#define MQTTWillQos                 (0)
#define MQTTWillRetain              (MQTTRetain)
#define MQTTAvailablePayload        "online"
#define MQTTUnavailablePayload      "offline"
#define ConnectBrokerRetryInterval_ms (2000)

// DHT sensor properties
#define DHTGNDPin 7
#define DHTVCCPin 8
#define DHTPIN 9
#define DHTInvalidValue 'N'
#define TempReadLoopPeriod_ms         (60000)
#define DHTStringLen (7)

// Network properties
uint8_t const mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x03 };
IPAddress const ip(192, 168, 1, 16);
IPAddress const gateway(192, 168, 0, 1);
IPAddress const subnet(255,255,254,0);

// Class definitions
EthernetClient ethClient;
PubSubClient mqtt(MQTTBrokerIP, MQTTBrokerPort, ethClient);
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
RunningMedian<uint8_t> samples(MEDIAN_SAMPLE_COUNT);
dht DHT;

// Static variables
static uint32_t mqttActionTimer;
static uint32_t sonarPingTimer;
static uint32_t presenceDetectTimer;
static uint32_t tempReadTimer;
static unsigned long previous;
static char previousTemp[DHTStringLen];
static char previousHumidity[DHTStringLen];
static boolean currentPresenceDetect;
static uint8_t previousPresenceValue;
static boolean runPresenceDetectTimeout;

// Function prototypes
void mqttCallback (char* topic, byte* payload, unsigned int length);
static boolean mqttHandle (void);
static void advanceTimers (void);
static bool publishMQTTMessage (char const * const sMQTTSubscription, char const * const sMQTTData, bool retain = false);

void setup() 
{
  Serial.print(F("Atelier Multisensor MQTT interface "));
  Serial.println(F(VERSION));

  pinMode(DHTGNDPin, OUTPUT);
  pinMode(DHTVCCPin, OUTPUT);
  digitalWrite(DHTGNDPin, LOW);
  digitalWrite(DHTVCCPin, HIGH);

  pinMode(PRESENCE_LED_PIN_SINK, OUTPUT);
  digitalWrite(DHTVCCPin, HIGH);

  Serial.begin(UARTBAUD);

  DHT.setDisableIRQ(true);

  // Initializes ethernet with DHCP
  Serial.println(F("Init Ethernet."));
  Ethernet.begin(mac, ip, MQTTBrokerIP, gateway, subnet);

  mqtt.setCallback(mqttCallback);

  mqttActionTimer = 0;
  sonarPingTimer = 0;
  tempReadTimer = 0;
  presenceDetectTimer = 0;
  previous = 0;
  previousTemp[0] = previousHumidity[0] = '\0';
  currentPresenceDetect = true;
  runPresenceDetectTimeout = true;
  previousPresenceValue = MAX_DISTANCE;
  digitalWrite(PRESENCE_LED_PIN_SINK, !currentPresenceDetect);
  Serial.println(F("Setup Complete."));
}

void loop() 
{
  advanceTimers();
  Ethernet.maintain();

  if(mqttHandle())
  {
    if(sonarPingTimer == 0)
    {
      sonarPingTimer = SonarPingLoopPeriod_ms;
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
            sonarPingTimer = SonarPingSleepPeriod_ms; // To not toggle presence state to many times in a short time.
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
        publishMQTTMessage(MQTTPubPresence, format);
      }
    }
    if(tempReadTimer == 0)
    {
      tempReadTimer = TempReadLoopPeriod_ms;

      DHT.read(DHTPIN);
      float const temp = DHT.temperature;
      float const hum = DHT.humidity;

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
      bool lenDiff = strlen(previousTemp) != strlen(format);
      bool stringDiff = 0 != strncmp(previousTemp, format, DHTStringLen);
      if(lenDiff || stringDiff)
      {
        if(DHTInvalidValue != format[0])
        {
          if(publishMQTTMessage(MQTTPubTemperature, format, MQTTRetain))
          {
            strncpy(previousTemp, format, DHTStringLen);
          }
        }
      }

      dtostrf(hum, -DHTStringLen, 2, format);
      for(uint8_t i = 0; i < DHTStringLen; i++)
      {
        if(format[i] == ' ')
        {
          format[i] = '\0';
          break;
        }
      }
      lenDiff = strlen(previousHumidity) != strlen(format);
      stringDiff = 0 != strncmp(previousHumidity, format, DHTStringLen);
      if(lenDiff || stringDiff)
      {
        if(DHTInvalidValue != format[0])
        {
          if(publishMQTTMessage(MQTTPubHumidity, format, MQTTRetain))
          {
            strncpy(previousHumidity, format, DHTStringLen);
          }
        }
      }
    }
  }
}

// Handles messages received in the mqttSubscribeTopic
void mqttCallback (char* topic, byte* payload, unsigned int length) 
{
  // Handles unused parameters
  (void)length;

  // Debug info
#define MQTTPayloadMaxExpectedSize (3) 
  char szTemp[MQTTPayloadMaxExpectedSize + sizeof('\0')];
  memset(szTemp, 0x00, MQTTPayloadMaxExpectedSize + sizeof('\0'));
  memcpy(szTemp, payload, length > MQTTPayloadMaxExpectedSize ? MQTTPayloadMaxExpectedSize : length);

  Serial.print(millis());
  Serial.print(F(": MQTT in : "));
  Serial.print(topic);
  Serial.print(F(" "));
  Serial.println(szTemp);
}

static boolean mqttHandle (void) 
{
  // If not MQTT connected, try connecting
  if(false == mqtt.connected())  
  {
    // Connect to MQTT broker, retry periodically
    if(0 == mqttActionTimer)
    {
      if(false == mqtt.connect(MQTTClientName, MQTTBrokerUser, MQTTBrokerPass, MQTTPubAvailable, MQTTWillQos, MQTTWillRetain, MQTTUnavailablePayload)) 
      {
        mqttActionTimer = ConnectBrokerRetryInterval_ms;
        Serial.println(F("MQTT connection failed."));
        Ethernet.begin(mac, ip, MQTTBrokerIP, gateway, subnet);
      } 
      else 
      {
        Serial.println(F("MQTT connected."));
        mqtt.subscribe(MQTTSubscribeTopic); 
        mqttActionTimer = 0;
        publishMQTTMessage(MQTTPubAvailable, MQTTAvailablePayload, MQTTRetain);
      }
    }
  }

  return mqtt.loop();
}
// Publish MQTT data to MQTT broker
static bool publishMQTTMessage (char const * const sMQTTSubscription, char const * const sMQTTData, bool retain)
{
  // Define and send message about door state
  bool result = mqtt.publish(sMQTTSubscription, sMQTTData, retain); 

  // Debug info
  Serial.print(millis());
  Serial.print(F(": MQTT Out : "));
  Serial.print(sMQTTSubscription);
  Serial.print(F(" "));
  Serial.println(sMQTTData);

  return result;
}

static void advanceTimers (void)
{
  unsigned long const current = millis();
  if(current != previous)
  {
    previous = current;

    if(mqttActionTimer)
    {
      mqttActionTimer--;
    }

    if(sonarPingTimer)
    {
      sonarPingTimer--;
    }

    if(tempReadTimer)
    {
      tempReadTimer--;
    }

    if((true == runPresenceDetectTimeout) && (presenceDetectTimer < SonarPingPresenceTimeout_ms))
    {
      presenceDetectTimer++;
    }
  }
}