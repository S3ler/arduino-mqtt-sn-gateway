#ifndef GATEWAY_MQTTMESSAGEHANDLERINTERFACE_H
#define GATEWAY_MQTTMESSAGEHANDLERINTERFACE_H


#include <cstdint>
#include "CoreInterface.h"
#include "LoggerInterface.h"

class Core;

class MqttMessageHandlerInterface {
public:
    virtual ~MqttMessageHandlerInterface() {};

    virtual bool begin() = 0;

    virtual void setCore(Core *core) = 0;

    virtual void setLogger(LoggerInterface *logger) = 0;

    virtual void setServer(uint8_t *ip, uint16_t port) = 0;

    virtual void setServer(const char *hostname, uint16_t port) = 0;

    // we always connect with the clean flag!
    virtual bool connect(const char *id) = 0;

    virtual bool connect(const char *id, const char *user, const char *pass) = 0;

    virtual bool
    connect(const char *id, const char *willTopic, uint8_t willQos, bool willRetain, const uint8_t *willMessage,
            const uint16_t willMessageLength) = 0;

    virtual bool
    connect(const char *id, const char *user, const char *pass, const char *willTopic, uint8_t willQos, bool willRetain,
            const uint8_t *willMessage, const uint16_t willMessageLength) = 0;

    virtual void disconnect() = 0;

    virtual bool publish(const char *topic, const uint8_t *payload, uint16_t plength, uint8_t qos, bool retained) = 0;

    virtual bool subscribe(const char *topic, uint8_t qos) = 0;

    virtual bool unsubscribe(const char *topic) = 0;

    /**
     * Call this method when you received a publish from the broker.
     * Implementation Note:
     * - Call publish in this method internally.
     * @param topic
     * @param payload
     * @param length
     * @return true if everthing worked fine, else otherwise.
     * // TODO adept message signature with retain and qos
     */
    virtual bool receive_publish(char* topic, uint8_t * payload, uint32_t length) = 0;

    virtual bool loop() = 0;

};


#endif //GATEWAY_MQTTMESSAGEHANDLERINTERFACE_H
