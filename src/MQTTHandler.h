#include "ExecTimer.h"
#include "secret.h"
#include "UIPEthernet.h"
#include <PubSubClient.h>

#pragma once

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
#define MQTTPubAvailable            "available"
#define MQTTPubPresence             "presence"
#define MQTTPubTemperature          "temperature"
#define MQTTPubHumidity             "humidity"
#define MQTTNotRetain               (false)
#define MQTTRetain                  (true)
#define MQTTWillQos                 (0)
#define MQTTWillRetain              (MQTTRetain)
#define MQTTAvailablePayload        "online"
#define MQTTUnavailablePayload      "offline"
#define ConnectBrokerRetryInterval_ms (2000)

class IPublish
{
public:
    virtual ~IPublish() {}

    virtual bool publish (char const * const topicSuffix, char const * const sMQTTData, bool retain = false) = 0;
};


class MQTTHandler : public IPublish, private ExecTimer
{
public:
    static IPublish * publisher;
    MQTTHandler (void);

    void init (void);

    bool execute (bool newMs);

    virtual bool publish (char const * const topicSuffix, char const * const sMQTTData, bool retain = false);

private:
    static auto const SubTopicMaxLength = 12;
    static auto const TopicPublishMaxLength = strlen(MQTTTopicPrefix) + strlen(MQTTTopicSet) + strlen("/") + SubTopicMaxLength;
    static void mqttCallback (char* topic, byte* payload, unsigned int length);

    EthernetClient ethClient;
    PubSubClient pubSubClient;
};