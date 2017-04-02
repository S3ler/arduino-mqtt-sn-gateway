//
// Created by bele on 09.12.16.
//

#ifndef GATEWAY_MQTTSNMESSAGEHANDLER_H
#define GATEWAY_MQTTSNMESSAGEHANDLER_H


#include <cstdint>
#include "CoreInterface.h"
#include "global_defines.h"
#include "mqttsn_messages.h"
#include "SocketInterface.h"


class SocketInterface;
class Core;
/**
 * Parses and routes incoming message to the Core class and/r answers mqtt-sn message immediately if possible.
 * Usage of this class:
 *  // Your SocketInterface implementation
 *  MySocker socket;
 *  // Core instance
 *  Core core;
 *  // Create instance
 *  MqttSnMessageHandler msg_handler;
 *  // set gw_id, network layer aka socket, and core
 *  msg_handler.setGwId(0x05);
 *  msg_handler.setSocket(&socket);
 *  msg_handler.setCore(&core);
 *  // call begin()
 *  msg_handler.begin();
 *  //everytime a message arrives call receive
 *  msg_hander.receive(bytes, bytes_length);
 *
 * A typical sequence for handling a message after a call to receive is:
 * parse message
 *  parses a message
 *  calls handle only if message is well formed to the mqtt-sn standard
 * handle message type
 *  call core functionality
 *  evaluate core functionality
 *  call send function with appropriated values
 * send reply
 *  send bytes to the underlying socket
 *
 * Not all steps are performed at each call, depend on the mqtt-sn standard.
 * E.g. for a PingReq message the sequence is only:
 *  parse message - process if valid
 *  handle message - call send_pingreq directly
 *  send reply - create a ping reply message and let the socket send it back
 *
 * Handles incoming messages via call on receiveData and calls then a function on Core and/or on Socket, depending on the message type.
 *
 * basic sequence_ receiveData -> parse_messagetype -> handle_messagetype (-> CRUD Persistent) -> send_messageresponse(optional)
 * will topic update not able
 * error during parsind lead to an complete ignore of the message
 * Quality of Service 2 is not supported at the moment
 */
class MqttSnMessageHandler {

private:
    SocketInterface *socket = nullptr;
    Core *core = nullptr;
    LoggerInterface *logger = nullptr;


public:

    bool begin();

    void setSocket(SocketInterface *socket);

    void setCore(Core *core);

    void receiveData(device_address *address, uint8_t *bytes);

private:

    void parse_searchgw(device_address *address, uint8_t *bytes);

    void handle_searchgw(device_address *address, uint8_t radius);

    void parse_connect(device_address *address, uint8_t *bytes);

    void handle_connect(device_address *address, const char *client_id, uint16_t duration, bool will = false,
                        bool clean_session = false);

    void parse_willtopic(device_address *address, uint8_t *bytes);

    void handle_willtopic(device_address *address, char *will_topic, bool retain, uint8_t qos);

    void parse_willmsg(device_address *address, uint8_t *bytes);

    void handle_willmsg(device_address *address, uint8_t *will_msg, uint8_t will_msg_len);

    void parse_register(device_address *address, uint8_t *bytes);

    void handle_register(device_address *address, uint16_t msg_id, char *topic_name);

    void parse_regack(device_address *address, uint8_t *bytes);

    void handle_regack(device_address *address, uint16_t topic_id, uint16_t msg_id, return_code_t return_code);

    void parse_publish(device_address *address, uint8_t *bytes);

    void handle_publish(device_address *address, uint8_t *data, uint16_t data_len,
                        uint16_t msg_id, uint16_t topic_id,
                        bool short_topic, bool retain, int8_t qos, bool dup = false);

    void parse_puback(device_address *address, uint8_t *bytes);

    void handle_puback(device_address *address, uint16_t msg_id, uint16_t topic_id, return_code_t return_code);

    void parse_subscribe(device_address *address, uint8_t *bytes);

    void handle_subscribe(device_address *address, uint16_t topic_id, uint16_t msg_id, bool short_topic, uint8_t qos,
                          bool dup = false);

    void handle_subscribe(device_address *address, char *topic_name, uint16_t msg_id, uint8_t qos, bool dup = false);

    void parse_unsubscribe(device_address *address, uint8_t *bytes);

    void handle_unsubscribe(device_address *address, uint16_t topic_id, uint16_t msg_id, bool short_topic, bool dup);

    void handle_unsubscribe(device_address *address, char *topic_name, uint16_t msg_id, bool dup);

    void parse_pingreq(device_address *address, uint8_t *bytes);

    void handle_pingreq(device_address *address);

    void handle_pingreq(device_address *address, char *client_id);

    void parse_disconnect(device_address *address, uint8_t *bytes);

    void handle_disconnect(device_address *address);

    void handle_disconnect(device_address *address, uint16_t duration);


public:

    void send_advertise(device_address *address, uint8_t gw_id, uint16_t duration);

    void send_gwinfo(device_address *address, uint8_t radius, uint8_t gw_id, uint8_t *gw_add, uint8_t gw_add_len);

    void send_connack(device_address *address, return_code_t return_code);

    void send_willtopicreq(device_address *address);

    void send_willmsgreq(device_address *address);

    void send_register(device_address *address, uint16_t topic_id, uint16_t msg_id, const char *topic_name);

    void send_regack(device_address *address, uint16_t topic_id, uint16_t msg_id, return_code_t return_code);

    bool send_publish(device_address *address, uint8_t *data, uint8_t data_len, uint16_t msg_id, uint16_t topic_id,
                      bool short_topic, bool retain, uint8_t qos, bool dup = false);

    void send_puback(device_address *address, uint16_t msg_id, uint16_t topic_id, return_code_t return_code);

    void
    send_suback(device_address *address, uint8_t qos, uint16_t topic_id, uint16_t msg_id, return_code_t return_code);

    void send_unsuback(device_address *address, uint16_t msg_id);

    void send_pingresp(device_address *address);

    void send_disconnect(device_address *address);

    void send_disconnect(device_address *address, uint16_t duration);

    void send_pubrec(device_address *address, uint16_t msg_id);

public:
    /**
     * Use this method from the SocketInterface side to notify the ther components about any completely broke network stack.
     */
    void notify_socket_disconnected();

    uint8_t get_maximum_publish_payload_length();

    void notify_socket_connected();

    void setLogger(LoggerInterface *logger);

};


#endif //GATEWAY_MQTTSNMESSAGEHANDLER_H
