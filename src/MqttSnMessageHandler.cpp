#include "MqttSnMessageHandler.h"

#define MQTTSN_DEBUG 1
#if MQTTSN_DEBUG

#include <Arduino.h>

#endif

bool MqttSnMessageHandler::begin() {
    if (core == nullptr || socket == nullptr || logger == nullptr) {
        return false;
    }

    if (socket->getMaximumMessageLength() <= 7) {
#if MQTTSN_DEBUG
        logger->log("Cannot start Mqtt-SN Message Handler: socket's maximum message length is smaller then 7", 0);
#endif
        return false;
    }
    // nothing to initialize
    return socket->begin();
}

void MqttSnMessageHandler::setCore(Core *core) {
    this->core = core;
}

void MqttSnMessageHandler::setSocket(SocketInterface *socket) {
    this->socket = socket;
}


void MqttSnMessageHandler::setLogger(LoggerInterface *logger) {
    this->logger = logger;
}


void MqttSnMessageHandler::receiveData(device_address *address, uint8_t *bytes) {
    message_header *header = (message_header *) bytes;
    if (header->length < 2) {
        return;
    }
    switch (header->type) {
        case MQTTSN_PINGREQ:
            parse_pingreq(address, bytes);
            break;
        case MQTTSN_SEARCHGW:
            parse_searchgw(address, bytes);
            break;
        case MQTTSN_CONNECT:
            parse_connect(address, bytes);
            break;
        case MQTTSN_WILLTOPIC:
            parse_willtopic(address, bytes);
            break;
        case MQTTSN_WILLMSG:
            parse_willmsg(address, bytes);
            break;
        case MQTTSN_REGISTER:
            parse_register(address, bytes);
            break;
        case MQTTSN_REGACK:
            parse_regack(address, bytes);
            break;
        case MQTTSN_PUBLISH:
            parse_publish(address, bytes);
            break;
        case MQTTSN_PUBACK:
            parse_puback(address, bytes);
            break;
        case MQTTSN_SUBSCRIBE:
            parse_subscribe(address, bytes);
            break;
        case MQTTSN_UNSUBSCRIBE:
            // TODO not implemented and tested
            parse_unsubscribe(address, bytes);
            break;
        case MQTTSN_DISCONNECT:
            parse_disconnect(address, bytes);
        case MQTTSN_WILLTOPICUPD:
            // TODO not implemented
            break;
        case MQTTSN_WILLMSGUPD:
            // TODO not implemented
            break;
            //case MQTTSN_FORWARD_ENCAPLSULATION:
            // TODO not implemented
            // parse&handle advertise packets and save them
            // TODO not implemented
        default:
            break;
    }
}

void MqttSnMessageHandler::parse_searchgw(device_address *address, uint8_t *bytes) {
    msg_searchgw *msg = (msg_searchgw *) bytes;
    if (bytes[0] == 3 && msg->type == MQTTSN_SEARCHGW) {
        handle_searchgw(address, msg->radius);
    }
}

void MqttSnMessageHandler::handle_searchgw(device_address *address, uint8_t radius) {
    device_address *own_address = socket->getAddress();
    uint8_t gw_id = 0;
    CORE_RESULT gw_id_result = core->get_gateway_id(&gw_id);
    if (gw_id_result == SUCCESS && gw_id > 0) {
        send_gwinfo(address, radius, gw_id, (uint8_t *) own_address, sizeof(own_address));
    }
}


void MqttSnMessageHandler::send_gwinfo(device_address *address, uint8_t radius, uint8_t gw_id, uint8_t *gw_add,
                                       uint8_t gw_add_len) {
    msg_gwinfo to_send(gw_id, gw_add);
    if (!socket->send(address, (uint8_t *) &to_send, gw_add_len, radius)) {
        core->notify_mqttsn_disconnected();
    }
}

void MqttSnMessageHandler::parse_connect(device_address *address, uint8_t *bytes) {
    msg_connect *msg = (msg_connect *) bytes;
    if (bytes[0] > 6 && bytes[1] == MQTTSN_CONNECT && msg->length == (6 + strlen(msg->client_id) + 1) &&
        msg->protocol_id == PROTOCOL_ID) {
        bool clean_session = (msg->flags & FLAG_CLEAN) != 0;
        bool will = (msg->flags & FLAG_WILL) != 0;

        handle_connect(address, msg->client_id, msg->duration, will, clean_session);
    }

}

void MqttSnMessageHandler::send_connack(device_address *address, return_code_t return_code) {
    msg_connack to_send(return_code);
    if (!socket->send(address, (uint8_t *) &to_send, sizeof(msg_connack))) {
        core->notify_mqttsn_disconnected();
    }
}

void MqttSnMessageHandler::handle_connect(device_address *address, const char *client_id, uint16_t duration, bool will,
                                          bool clean_session) {
#if MQTTSN_DEBUG
    logger->log("MQTTSN_CONNECT", 3);
#endif
    CORE_RESULT add_result = core->add_client(client_id, duration, clean_session, address);
    if (add_result != SUCCESS) {
        if (add_result == FULL) {
            send_connack(address, REJECTED_CONGESTION);
        } else {
            send_connack(address, REJECTED_NOT_SUPPORTED);
        }
        return;
    }
    if (will) {
        CORE_RESULT await_result = core->await_message(address, MQTTSN_WILLTOPIC);
        if (await_result != SUCCESS) {
            send_connack(address, REJECTED_NOT_SUPPORTED);
            return;
        }
        send_willtopicreq(address);
        return;
    }
    send_connack(address, ACCEPTED);
}

void MqttSnMessageHandler::parse_willtopic(device_address *address, uint8_t *bytes) {
    msg_willtopic *msg = (msg_willtopic *) bytes;
    if (bytes[0] > 3 && bytes[1] == MQTTSN_WILLTOPIC && msg->length == (3 + strlen(msg->will_topic) + 1)) {
        int8_t qos = 0;
        if ((msg->flags & FLAG_QOS_M1) == FLAG_QOS_M1) {
            qos = -1;
        } else if ((msg->flags & FLAG_QOS_2) == FLAG_QOS_2) {
            qos = 2;
        } else if ((msg->flags & FLAG_QOS_1) == FLAG_QOS_1) {
            qos = 1;
        } else {
            qos = 0;
        }
        bool retain = (msg->flags & FLAG_WILL) != 0;
        handle_willtopic(address, msg->will_topic, retain, qos);
    }
}

void MqttSnMessageHandler::handle_willtopic(device_address *address, char *will_topic, bool retain, uint8_t qos) {
#if MQTTSN_DEBUG
    logger->log("MQTTSN_WILLTOPIC", 3);
#endif
    if (strlen(will_topic) == 0 && qos == 0 && !retain) {
        core->remove_will(address);
        return;
    } else {
        CORE_RESULT topic_result = core->add_will_topic(address, will_topic, retain, qos);
        if (topic_result != SUCCESS) {
            send_connack(address, REJECTED_NOT_SUPPORTED);
            return;
        }
    }
    CORE_RESULT await_result = core->await_message(address, MQTTSN_WILLMSG);
    if (await_result != SUCCESS) {
        send_connack(address, REJECTED_NOT_SUPPORTED);
        return;
    }
    send_willmsgreq(address);
}

void MqttSnMessageHandler::send_willtopicreq(device_address *address) {
    message_header to_send;
    to_send.length = 2;
    to_send.type = MQTTSN_WILLTOPICREQ;
    if (!socket->send(address, (uint8_t *) &to_send, sizeof(message_header))) {
        core->notify_mqttsn_disconnected();
    }
}

void MqttSnMessageHandler::send_willmsgreq(device_address *address) {
    message_header to_send;
    to_send.length = 2;
    to_send.type = MQTTSN_WILLMSGREQ;
    if (!socket->send(address, (uint8_t *) &to_send, sizeof(message_header))) {
        core->notify_mqttsn_disconnected();
    }
}

void MqttSnMessageHandler::parse_willmsg(device_address *address, uint8_t *bytes) {
    msg_willmsg *msg = (msg_willmsg *) bytes;
    if (bytes[0] > 2 && bytes[1] == MQTTSN_WILLMSG) {
        handle_willmsg(address, msg->willmsg, (msg->length - (uint8_t) 2));
    }
}

void MqttSnMessageHandler::handle_willmsg(device_address *address, uint8_t *will_msg, uint8_t will_msg_len) {
#if MQTTSN_DEBUG
    logger->log("MQTTSN_WILLMSG", 3);
#endif

    if (core->add_will_msg(address, will_msg, will_msg_len) == SUCCESS) {
#if MQTTSN_DEBUG
        logger->log("ACCEPTED", 3);
#endif
        send_connack(address, ACCEPTED);
    } else {
#if MQTTSN_DEBUG
        logger->log("REJECTED_NOT_SUPPORTED", 3);
#endif
        send_connack(address, REJECTED_NOT_SUPPORTED);
    }
}

void MqttSnMessageHandler::send_advertise(device_address *address, uint8_t gw_id, uint16_t duration) {
    msg_advertise to_send(gw_id, duration);
    if (!socket->send(address, (uint8_t *) &to_send, sizeof(msg_advertise))) {
        core->notify_mqttsn_disconnected();
    }
}

void MqttSnMessageHandler::parse_register(device_address *address, uint8_t *bytes) {
    msg_register *msg = (msg_register *) bytes;
    if (bytes[0] > 6 && bytes[1] == MQTTSN_REGISTER && msg->topic_id == 0x0000 &&
        msg->length == (6 + strlen(msg->topic_name) + 1)) {
        handle_register(address, msg->message_id, msg->topic_name);
    }
}

void MqttSnMessageHandler::handle_register(device_address *address, uint16_t msg_id, char *topic_name) {
#if MQTTSN_DEBUG
    logger->log("MQTTSN_REGISTER", 3);
#endif

    uint16_t topic_id = 0;
    uint16_t *p_topic_id = &topic_id;

    CORE_RESULT register_result = core->register_topic(address, topic_name, p_topic_id);
    if (register_result == FULL) {
        send_regack(address, 0x0000, msg_id, REJECTED_CONGESTION);
        return;
    }
    if (register_result != SUCCESS) {
        send_regack(address, 0x0000, msg_id, REJECTED_NOT_SUPPORTED);
        return;
    }
    send_regack(address, topic_id, msg_id, ACCEPTED);
}

void
MqttSnMessageHandler::send_regack(device_address *address, uint16_t topic_id, uint16_t msg_id,
                                  return_code_t return_code) {
#if MQTTSN_DEBUG
    logger->start_log("send regack - topic id ", 3);
    char uint16_buf[8];
    sprintf(uint16_buf, "%d", topic_id);
    logger->append_log(uint16_buf);
    logger->append_log(" msg id");
    sprintf(uint16_buf, "%d", msg_id);
    logger->append_log(uint16_buf);
    switch (return_code) {
        case ACCEPTED:
            logger->append_log(" ACCEPTED");
            break;
        case REJECTED_CONGESTION:
            logger->append_log(" REJECTED_CONGESTION");
            break;
        case REJECTED_INVALID_TOPIC_ID:
            logger->append_log(" REJECTED_INVALID_TOPIC_ID");
            break;
        case REJECTED_NOT_SUPPORTED:
            logger->append_log(" REJECTED_NOT_SUPPORTED");
            break;
        default:
            break;
    }
#endif
    msg_regack to_send(topic_id, msg_id, return_code);
    if (!socket->send(address, (uint8_t *) &to_send, to_send.length)) {
        core->notify_mqttsn_disconnected();
    }
}

void
MqttSnMessageHandler::send_register(device_address *address, uint16_t topic_id, uint16_t msg_id, const char *topic_name) {
    msg_register to_send(topic_id, msg_id, topic_name);
    if (!socket->send(address, (uint8_t *) &to_send, sizeof(msg_register) + sizeof(topic_name))) {
        core->notify_mqttsn_disconnected();
    }
}

void MqttSnMessageHandler::parse_regack(device_address *address, uint8_t *bytes) {
    msg_regack *msg = (msg_regack *) bytes;
    if (bytes[0] == 7 && bytes[1] == MQTTSN_REGACK) {
        handle_regack(address, msg->topic_id, msg->message_id, msg->return_code);
    }
}

void
MqttSnMessageHandler::handle_regack(device_address *address, uint16_t topic_id, uint16_t msg_id,
                                    return_code_t return_code) {
    CORE_RESULT result = core->notify_regack_arrived(address, topic_id, msg_id, return_code);
    if (result != SUCCESS) {
        send_disconnect(address);
    }
}

void MqttSnMessageHandler::parse_publish(device_address *address, uint8_t *bytes) {

    msg_publish *msg = (msg_publish *) bytes;
    if (bytes[0] > 7 && bytes[1] == MQTTSN_PUBLISH) { // 7 bytes header + at least 1 byte data
        bool dup = (msg->flags & FLAG_DUP) != 0;

        int8_t qos = 0;
        if ((msg->flags & FLAG_QOS_M1) == FLAG_QOS_M1) {
            qos = -1;
        } else if ((msg->flags & FLAG_QOS_2) == FLAG_QOS_2) {
            qos = 2;
        } else if ((msg->flags & FLAG_QOS_1) == FLAG_QOS_1) {
            qos = 1;
        } else {
            qos = 0;
        }

        bool retain = (msg->flags & FLAG_RETAIN) != 0;
        bool short_topic = (msg->flags & FLAG_TOPIC_SHORT_NAME) != 0;
        uint16_t data_len = bytes[0] - (uint8_t) 7;
        if (((qos == 0) || (qos == -1)) && msg->message_id != 0x0000) {
            // this can be too strict
            // we can also ignore the message_id for Qos 0 and -1
            return;
        }

        if (!short_topic && !(msg->flags & FLAG_TOPIC_PREDEFINED_ID != 0)) { // TODO what does this so?! WTF?!
            handle_publish(address, msg->data, data_len, msg->message_id, msg->topic_id, short_topic, retain, qos, dup);
        }
        handle_publish(address, msg->data, data_len, msg->message_id, msg->topic_id, short_topic, retain, qos, dup);
    }
}

void MqttSnMessageHandler::handle_publish(device_address *address, uint8_t *data, uint16_t data_len, uint16_t msg_id,
                                          uint16_t topic_id, bool short_topic, bool retain, int8_t qos, bool dup) {
#if MQTTSN_DEBUG
    logger->log("MQTTSN_PUBLISH", 3);
#endif
    uint16_t new_topic_id = 0x0;
    uint16_t *p_new_topic_id = &new_topic_id;
    CORE_RESULT result_publish = core->publish(address, data, data_len, msg_id, topic_id, short_topic,
                                               retain, qos, dup, p_new_topic_id);
    if (qos == -1) {
        return;
    }
    if (result_publish != SUCCESS && result_publish != TOPICIDNONEXISTENCE) { // aka client map error

        if (qos == 1) {
            send_puback(address, msg_id, topic_id, REJECTED_NOT_SUPPORTED);
            return;
        }
        if (result_publish == CLIENTNONEXISTENCE) {
            send_disconnect(address);
            return;
        }
        // TODO here can happen a problem
        // when mqtt connection is down, and we have a client with outstanding publishes.
        // if we disconnect the publishes are gone
        // but if we use qos 0 publishes, there is no other way to tell the client that something went wrong.
        send_disconnect(address);
        return;
    }
    if (qos == 0) {
        if (result_publish == TOPICIDNONEXISTENCE) {
            send_puback(address, msg_id, topic_id, REJECTED_INVALID_TOPIC_ID);
            return;
        }
        return;
    }
    if (qos == 1) {
        if (result_publish == TOPICIDNONEXISTENCE) {
            send_puback(address, msg_id, topic_id, REJECTED_INVALID_TOPIC_ID);
            return;
        }

        send_puback(address, msg_id, topic_id, ACCEPTED);
        return;
    }
    if (qos == 2) {
        CORE_RESULT await_result = core->await_message(address, MQTTSN_PUBREL);
        if (await_result != SUCCESS) {
            send_disconnect(address);
            return;
        }
        send_pubrec(address, msg_id);
        return;
    }

}


void MqttSnMessageHandler::send_disconnect(device_address *address) {
    message_header to_send;
    to_send.to_disconnect();
    if (!socket->send(address, (uint8_t *) &to_send, sizeof(message_header))) {
        core->notify_mqttsn_disconnected();
    }
}

void MqttSnMessageHandler::send_disconnect(device_address *address, uint16_t duration) {
    msg_disconnect to_send = msg_disconnect();
    to_send.to_disconnect();
    to_send.length = 4;
    to_send.duration = duration;
    if (!socket->send(address, (uint8_t *) &to_send, sizeof(message_header))) {
        core->notify_mqttsn_disconnected();
    }
}

void
MqttSnMessageHandler::send_puback(device_address *address, uint16_t msg_id, uint16_t topic_id,
                                  return_code_t return_code) {
    msg_puback to_send(topic_id, msg_id, return_code);
    if (!socket->send(address, (uint8_t *) &to_send, sizeof(msg_puback))) {
        core->notify_mqttsn_disconnected();
    }
}

void MqttSnMessageHandler::parse_subscribe(device_address *address, uint8_t *bytes) {

    msg_subscribe *msg = (msg_subscribe *) bytes;
    if (bytes[0] > 3) {
        bool short_topic_or_predefined =
                ((msg->flags & FLAG_TOPIC_SHORT_NAME) != 0) || ((msg->flags & FLAG_TOPIC_PREDEFINED_ID) != 0);
        int8_t qos = 0;
        if ((msg->flags & FLAG_QOS_M1) == FLAG_QOS_M1) {
            qos = -1;
        } else if ((msg->flags & FLAG_QOS_2) == FLAG_QOS_2) {
            qos = 2;
        } else if ((msg->flags & FLAG_QOS_1) == FLAG_QOS_1) {
            qos = 1;
        } else {
            qos = 0;
        }
        if (qos == -1) {
            // you cannot subscribe with qos -1
            return;
        }
        bool dup = (msg->flags & FLAG_DUP) != 0;
        if (short_topic_or_predefined && bytes[0] == 7) {
            msg_subscribe_shorttopic *msg_short = (msg_subscribe_shorttopic *) bytes;
            bool short_topic = (msg->flags & FLAG_TOPIC_SHORT_NAME) != 0;
            handle_subscribe(address, msg_short->topic_id, msg_short->message_id, short_topic, (uint8_t) qos, dup);
        } else if (bytes[0] > 6) {
            msg_subscribe_topicname *msg_topicname = (msg_subscribe_topicname *) bytes;
            handle_subscribe(address, msg_topicname->topic_name, msg_topicname->message_id, (uint8_t) qos, dup);
        }
    }

}

void MqttSnMessageHandler::send_suback(device_address *address, uint8_t qos, uint16_t topic_id, uint16_t msg_id,
                                       return_code_t return_code) {
    msg_suback to_send(qos, topic_id, msg_id, return_code);
    if (!socket->send(address, (uint8_t *) &to_send, sizeof(msg_suback))) {
        core->notify_mqttsn_disconnected();
    }
}

void MqttSnMessageHandler::parse_unsubscribe(device_address *address, uint8_t *bytes) {
    msg_unsubscribe *msg = (msg_unsubscribe *) bytes;
    if (bytes[0] > 3) {
        bool short_topic_or_predefined =
                ((msg->flags & FLAG_TOPIC_SHORT_NAME) != 0) || ((msg->flags & FLAG_TOPIC_PREDEFINED_ID) != 0);
        if (short_topic_or_predefined && bytes[0] == 7) {
            bool short_topic = (msg->flags & FLAG_TOPIC_SHORT_NAME) != 0;
            bool dup = (msg->flags & FLAG_DUP) != 0;
            handle_unsubscribe(address, msg->topic_id, msg->message_id, short_topic, dup);
        } else if (bytes[0] > 6) {
            bool dup = (msg->flags & FLAG_DUP) != 0;
            handle_unsubscribe(address, msg->topic_name, msg->message_id, dup);
        }
    }
}

void
MqttSnMessageHandler::handle_unsubscribe(device_address *address, uint16_t topic_id, uint16_t msg_id, bool short_topic,
                                         bool dup) {
    CORE_RESULT unsubscribe_result = core->delete_subscription(address, topic_id, msg_id, short_topic, dup);
    if (unsubscribe_result != SUCCESS) {
        if (unsubscribe_result == CLIENTNONEXISTENCE) {
            send_disconnect(address);
        } else {
            send_unsuback(address, msg_id);
        }
        return;
    }
    send_unsuback(address, msg_id);
}

void MqttSnMessageHandler::send_unsuback(device_address *address, uint16_t msg_id) {
    msg_unsuback to_send(msg_id);
    if (!socket->send(address, (uint8_t *) &to_send, sizeof(msg_unsuback))) {
        core->notify_mqttsn_disconnected();
    }
}

void
MqttSnMessageHandler::handle_subscribe(device_address *address, uint16_t topic_id, uint16_t msg_id, bool short_topic,
                                       uint8_t qos, bool dup) {
#if MQTTSN_DEBUG
    logger->start_log("MQTTSN_SUBSCRIBE short_topic", 3);
    char uint16_buf[10];

    sprintf(uint16_buf, "%d", topic_id);
    logger->append_log(" topic_id ");
    logger->append_log(uint16_buf);

    sprintf(uint16_buf, "%d", msg_id);
    logger->append_log(" msg_id ");
    logger->append_log(uint16_buf);

    logger->append_log(" short_topic ");
    logger->append_log(short_topic ? "true" : "false");
    logger->append_log(" qos ");
    sprintf(uint16_buf, "%d", qos);
    logger->append_log(uint16_buf);

#endif
    uint8_t granted_qos = 0;
    uint16_t new_topic_id = 0;
    CORE_RESULT subscribe_result = core->add_subscription(address, topic_id, msg_id, short_topic, qos, dup,
                                                          &new_topic_id, &granted_qos);
    if (subscribe_result != SUCCESS) {
        // when the subscription is rejected
        // which values the other values have is not defined in the standard
        // i suggest 0
        if (subscribe_result == CLIENTSUBSCRIPTIONFULL) { // REJECTED_CONGESTION
            send_suback(address, 0, 0, msg_id, REJECTED_CONGESTION);
        } else if (subscribe_result == TOPICIDNONEXISTENCE) { // REJECTED_INVALID_TOPIC_ID
            send_suback(address, 0, 0, msg_id, REJECTED_INVALID_TOPIC_ID);
        } else if (subscribe_result == CLIENTNONEXISTENCE) {
            send_disconnect(address);
        } else { //REJECTED_NOT_SUPPORTED
            send_suback(address, 0, 0, msg_id, REJECTED_NOT_SUPPORTED);
        }
        return;
    }
#if MQTTSN_DEBUG
    sprintf(uint16_buf, "%d", new_topic_id);
    logger->append_log(" new_topic_id ");
    logger->append_log(uint16_buf);

    sprintf(uint16_buf, "%d", granted_qos);
    logger->append_log(" granted_qos ");
    logger->append_log(uint16_buf);
    logger->append_log(" - ACCEPTED");
#endif
    //TODO topic id = 0 should never happen!
    send_suback(address, granted_qos, new_topic_id, msg_id, ACCEPTED);
}

void
MqttSnMessageHandler::handle_subscribe(device_address *address, char *topic_name, uint16_t msg_id, uint8_t qos,
                                       bool dup) {
    uint16_t new_topic_id = 0;
    CORE_RESULT register_result = core->register_topic(address, topic_name, &new_topic_id);
    if (register_result == FULL) {
        send_suback(address, qos, 0, msg_id, REJECTED_CONGESTION);
        return;
    }
    if (register_result != SUCCESS) {
        send_suback(address, qos, 0, msg_id, REJECTED_NOT_SUPPORTED);
        return;
    }

    bool short_topic = true;
    uint8_t granted_qos = 0;

    CORE_RESULT subscribe_result = core->add_subscription(address, new_topic_id, msg_id, short_topic, qos, dup,
                                                          &new_topic_id, &granted_qos);
    if (subscribe_result != SUCCESS) {
        // when the subscription is rejected
        // which values the other values have is not defined in the standard
        // i suggest 0
        if (subscribe_result == CLIENTSUBSCRIPTIONFULL) { // REJECTED_CONGESTION
            send_suback(address, 0, 0, msg_id, REJECTED_CONGESTION);
        } else if (subscribe_result == TOPICIDNONEXISTENCE) { // REJECTED_INVALID_TOPIC_ID
            send_suback(address, 0, 0, msg_id, REJECTED_INVALID_TOPIC_ID);
        } else if (subscribe_result == CLIENTNONEXISTENCE) {
            send_disconnect(address);
        } else { //REJECTED_NOT_SUPPORTED
            send_suback(address, 0, 0, msg_id, REJECTED_NOT_SUPPORTED);
        }
        return;
    }
#if MQTTSN_DEBUG
    char uint16_buf[10];

    sprintf(uint16_buf, "%d", new_topic_id);
    logger->append_log(" new_topic_id ");
    logger->append_log(uint16_buf);

    sprintf(uint16_buf, "%d", granted_qos);
    logger->append_log(" granted_qos ");
    logger->append_log(uint16_buf);
    logger->append_log(" - ACCEPTED");
#endif
    send_suback(address, granted_qos, new_topic_id, msg_id, ACCEPTED);
}

void MqttSnMessageHandler::handle_unsubscribe(device_address *address, char *topic_name, uint16_t msg_id, bool dup) {

    CORE_RESULT unsubscribe_result = core->delete_subscription(address, topic_name, msg_id, dup);
    if (unsubscribe_result != SUCCESS) {
        if (unsubscribe_result == CLIENTNONEXISTENCE) {
            send_disconnect(address);
        } else {
            send_unsuback(address, msg_id);
        }
        return;
    }
    send_unsuback(address, msg_id);
}

void MqttSnMessageHandler::parse_pingreq(device_address *address, uint8_t *bytes) {
    msg_pingreq *msg = (msg_pingreq *) bytes;
    if (bytes[0] == 2) {
        handle_pingreq(address);
    } else if (bytes[0] > 2 && strlen(msg->client_id) < 24 && strlen(msg->client_id) == (msg->length - 3)) {
        handle_pingreq(address, msg->client_id);
    }
}

void MqttSnMessageHandler::handle_pingreq(device_address *address) {
    //core->reset_timeout(address);
    send_pingresp(address);
}

void MqttSnMessageHandler::send_pingresp(device_address *address) {
    message_header to_send;
    to_send.length = 2;
    to_send.type = MQTTSN_PINGRESP;
    if (!socket->send(address, (uint8_t *) &to_send, sizeof(message_header))) {
        core->notify_mqttsn_disconnected();
    }
}

void MqttSnMessageHandler::handle_pingreq(device_address *address, char *client_id) {
    CORE_RESULT set_status_result = core->set_awake(address);
    if (set_status_result != SUCCESS) {
        send_disconnect(address);
    }
}

void MqttSnMessageHandler::parse_disconnect(device_address *address, uint8_t *bytes) {
    msg_disconnect *msg = (msg_disconnect *) bytes;
    if (bytes[0] == 2) {
        handle_disconnect(address);
    } else if (bytes[0] == 4) {
        handle_disconnect(address, msg->duration);
    }
}

void MqttSnMessageHandler::handle_disconnect(device_address *address) {
    CORE_RESULT disconnect_result = core->set_disconnected(address);
    if (disconnect_result != SUCCESS) {
        return;
    }
    send_disconnect(address);
}

void MqttSnMessageHandler::handle_disconnect(device_address *address, uint16_t duration) {
#if MQTTSN_DEBUG
    logger->log("MQTTSN_DISCONNECT", 3);
#endif
    // We enhance the standard a little bit here:
    // if we acknowledge a sleeping time disconnect, we answer with the duration
    // if something when terribly wrong, we disconnect the client by using disconnect without a flag
    // then the client know unambiguously if he can sleep or connect to a new gateway

    CORE_RESULT disconnect_result = core->set_asleep(address, duration);
    if (disconnect_result != SUCCESS) {
        send_disconnect(address);
        return;
    }
    send_disconnect(address, duration);
}

void MqttSnMessageHandler::parse_puback(device_address *address, uint8_t *bytes) {
    msg_puback *msg = (msg_puback *) bytes;
    if (bytes[0] == 7) {
        handle_puback(address, msg->topic_id, msg->message_id, msg->return_code);
    }
}

void MqttSnMessageHandler::handle_puback(device_address *address, uint16_t msg_id, uint16_t topic_id,
                                         return_code_t return_code) {
    CORE_RESULT result = core->notify_puback_arrived(address, msg_id, topic_id, return_code);
    if (result != SUCCESS) {
        send_disconnect(address);
    }
}

bool MqttSnMessageHandler::send_publish(device_address *address, uint8_t *data, uint8_t data_len, uint16_t msg_id,
                                        uint16_t topic_id, bool short_topic, bool retain, uint8_t qos, bool dup) {
    msg_publish to_send(dup, qos, retain, short_topic, topic_id, msg_id, data, data_len);
    bool send_status = socket->send(address, (uint8_t *) &to_send, to_send.length);
    if (!send_status) {
        core->notify_mqttsn_disconnected();
    }
    return send_status;
}

void MqttSnMessageHandler::send_pubrec(device_address *address, uint16_t msg_id) {
    msg_pubrec to_send(msg_id);
    if (!socket->send(address, (uint8_t *) &to_send, sizeof(msg_pubrec))) {
        core->notify_mqttsn_disconnected();
    }
}

void MqttSnMessageHandler::notify_socket_disconnected() {
    core->notify_mqttsn_disconnected();
}

void MqttSnMessageHandler::notify_socket_connected() {
    core->notify_mqttsn_connected();
}


uint8_t MqttSnMessageHandler::get_maximum_publish_payload_length() {
    if (socket->getMaximumMessageLength() <= 7) {
        return 0;
    }
    return (socket->getMaximumMessageLength() - (uint8_t) 7);
}



