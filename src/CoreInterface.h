#ifndef GATEWAY_CORE_H
#define GATEWAY_CORE_H

#include "core_defines.h"
#include "global_defines.h"
#include "mqttsn_messages.h"
#include "PersistentInterface.h"
#include "MqttMessageHandlerInterface.h"
#include "MqttSnMessageHandler.h"
#include "System.h"

/**
 * Core message return Zero on an unexpected error
 */

class MqttMessageHandlerInterface;

class PersistentInterface;

class MqttSnMessageHandler;


class Core {
public:
    virtual ~Core() {};

    /**
     * Starts the database and initializes internal variables.
     * This mainly for comformances with other Arduino libraries.
     */
    virtual bool begin()=0;

    virtual void setPersistent(PersistentInterface *persistent)=0;

    virtual void setMqttMessageHandler(MqttMessageHandlerInterface *mqtt)=0;

    virtual void setMqttSnMessageHandler(MqttSnMessageHandler *mqttsn)=0;

    virtual void setLogger(LoggerInterface *logger)=0;

    virtual void setSystem(System* system)=0;

    virtual void loop() = 0;

    /**
     * Adds a new Mqtt-SN client to the database.
     * Identification of the client is done by the client_id.
     * Creates a new client in the database if not already exist.
     * If the clean_session is true it will first remove an existing client out of the databse, before adding the new client.
     *
     * @param client_id by which the client is uniquely identified
     * @param duration time period in seconds until the client is lost or removed
     * @param clean_session remove existing client with client_id if exist
     * @param address by which the client can be addresses if no socket is supported
     * @return
     * - SUCCESS if everything went fine
     * - FULL if no more space for further clients is available
     * - ZERO for any other errors
     */
    virtual CORE_RESULT add_client(const char *client_id, uint16_t duration, bool clean_session,
                                   device_address *address) =0;

    virtual CORE_RESULT remove_will(device_address *address)=0;

    virtual CORE_RESULT add_will_topic(device_address *address, char *will_topic, bool retain, uint8_t qos)=0;

    virtual CORE_RESULT add_will_msg(device_address *address, uint8_t *will_msg, uint8_t will_msg_len)=0;

    virtual CORE_RESULT await_message(device_address *address, message_type type)=0;

    virtual CORE_RESULT register_topic(device_address *address, char *topic_name, uint16_t *p_topic_id)=0;

    virtual CORE_RESULT
    notify_regack_arrived(device_address *address, uint16_t topic_id, uint16_t msg_id, return_code_t return_code)=0;

/**
 *
 * @param address
 * @param data
 * @param data_len
 * @param msg_id
 * @param topic_id
 * @param short_topic
 * @param retain
 * @param qos
 * @param dup
 * @param new_topic_id
 * @return
 * - SUCCESS if everything went fine
 * - CLIENTNONEXISTENCE if the client is not connected
 * - TOPICIDNONEXISTENCE if the give topic_id does not exist
 * - ZERO for any other error
 */
    virtual CORE_RESULT
    publish(device_address *address, const uint8_t *data, uint16_t data_len, uint16_t msg_id, uint16_t topic_id,
            bool short_topic,
            bool retain, int8_t qos, bool dup, uint16_t *new_topic_id)=0;

    virtual CORE_RESULT add_subscription(device_address *address, uint16_t topic_id, uint16_t msg_id, bool short_topic, uint8_t qos, bool dup,uint16_t* new_topic_id,uint8_t* granted_qos) = 0;

    virtual CORE_RESULT
    delete_subscription(device_address *address, uint16_t topic_id, uint16_t msg_id, bool short_topic, bool dup)=0;

    virtual CORE_RESULT
    delete_subscription(device_address *address, char* topic_name, uint16_t msg_id, bool dup)=0;

    virtual CORE_RESULT set_disconnected(device_address *address)=0;

    virtual CORE_RESULT set_asleep(device_address *address, uint16_t duration)=0;

    virtual CORE_RESULT set_asleep(device_address *address)=0;

    virtual CORE_RESULT
    notify_puback_arrived(device_address *address, uint16_t msg_id, uint16_t topic_id, return_code_t return_code)=0;

    virtual CORE_RESULT set_awake(device_address *address)  = 0;



    virtual CORE_RESULT publish(char *topic_name, uint8_t *data, uint32_t data_length, bool retain) = 0;

public:    // gateway configuration

    virtual CORE_RESULT get_gateway_id(uint8_t* gateway_id)=0;

    virtual CORE_RESULT get_mqtt_config(uint8_t *server_ip, uint16_t *server_port, char *client_id) = 0;

    virtual CORE_RESULT get_mqtt_login_config(char *username, char *password) = 0;

    virtual CORE_RESULT get_mqtt_will(char *will_topic, char *will_msg, uint8_t *will_qos, bool *will_retain) = 0;

public:    // gateway connection status

    virtual CORE_RESULT notify_mqttsn_disconnected()=0;

    virtual CORE_RESULT notify_mqtt_disconnected()=0;

    virtual CORE_RESULT notify_mqtt_connected() = 0;

    virtual CORE_RESULT notify_mqttsn_connected() = 0;
};


#endif //GATEWAY_DATABASE_H
