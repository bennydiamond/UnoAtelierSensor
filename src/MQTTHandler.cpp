#include "MQTTHandler.h"

// Network properties
uint8_t const mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x04 };
IPAddress const ip(192, 168, 1, 17);
IPAddress const gateway(192, 168, 0, 1);
IPAddress const subnet(255,255,254,0);

IPublish * MQTTHandler::publisher;

MQTTHandler::MQTTHandler (void) :
ExecTimer::ExecTimer(ConnectBrokerRetryInterval_ms),
ethClient(),
pubSubClient(MQTTBrokerIP, MQTTBrokerPort, ethClient)
{
  publisher = this;
}

void MQTTHandler::init (void)
{
  Serial.println(F("Init ETH"));
  Ethernet.begin((uint8_t*)mac, ip, MQTTBrokerIP, gateway, subnet);

  pubSubClient.setCallback(mqttCallback);
}

bool MQTTHandler::execute (bool newMs)
{
  Ethernet.maintain();

  // If not MQTTHandler connected, try connecting
  if(false == pubSubClient.connected())  
  {
    // Connect to MQTTHandler broker, retry periodically
    if(newMillisecond(newMs))
    {
        if(false == pubSubClient.connect(MQTTClientName, MQTTBrokerUser, MQTTBrokerPass, MQTTPubAvailable, MQTTWillQos, MQTTWillRetain, MQTTUnavailablePayload)) 
        {
            Serial.println(F("MQTT connect fail"));
            setPeriod(ConnectBrokerRetryInterval_ms);
        } 
        else 
        {
            Serial.println(F("MQTT connected"));
            pubSubClient.subscribe(MQTTSubscribeTopic); 
            publish(MQTTPubAvailable, MQTTAvailablePayload, MQTTRetain);
        }
    }
  }

  return pubSubClient.loop();
}

bool MQTTHandler::publish (char const * const topicSuffix, char const * const sMQTTHandlerData, bool retain)
{
  char topic[TopicPublishMaxLength];
  snprintf(topic, TopicPublishMaxLength, MQTTTopicPrefix MQTTTopicSet "/%s", topicSuffix);
  bool result = pubSubClient.publish(topic, sMQTTHandlerData, retain); 

  // Debug info
  Serial.print(millis());
  Serial.print(F(": MQTTHandler Out : "));
  Serial.print(topic);
  Serial.print(F(" "));
  Serial.println(sMQTTHandlerData);

  return result;
}

// Handles messages received in the MQTTHandlerSubscribeTopic
void MQTTHandler::mqttCallback (char* topic, byte* payload, unsigned int length) 
{
  // Handles unused parameters
  (void)length;

  // Debug info
#define MQTTHandlerPayloadMaxExpectedSize (3) 
  char szTemp[MQTTHandlerPayloadMaxExpectedSize + sizeof('\0')];
  memset(szTemp, 0x00, MQTTHandlerPayloadMaxExpectedSize + sizeof('\0'));
  memcpy(szTemp, payload, length > MQTTHandlerPayloadMaxExpectedSize ? MQTTHandlerPayloadMaxExpectedSize : length);

  Serial.print(millis());
  Serial.print(F(": MQTTHandler in : "));
  Serial.print(topic);
  Serial.print(F(" "));
  Serial.println(szTemp);
}