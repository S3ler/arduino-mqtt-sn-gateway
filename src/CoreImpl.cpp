#include "CoreImpl.h"

#define CORE_LOG 1 // for logging during production
#define CORE_DEBUG 1 // for debugging
#if CORE_LOG || CORE_DEBUG

#include <Arduino.h>

#endif

bool CoreImpl::begin() {
    if (persistent != nullptr && mqtt != nullptr && mqttsn != nullptr && system != nullptr) {
        if (persistent->begin() && mqtt->begin() && mqttsn->begin()) {
            return true;
        }
    }
    return false;
}

void CoreImpl::setPersistent(PersistentInterface *persistent) {
    this->persistent = persistent;
}

void CoreImpl::setMqttMessageHandler(MqttMessageHandlerInterface *mqtt) {
    this->mqtt = mqtt;
}

void CoreImpl::setMqttSnMessageHandler(MqttSnMessageHandler *mqttsn) {
    this->mqttsn = mqttsn;
}

void CoreImpl::setLogger(LoggerInterface *logger) {
    this->logger = logger;
}

void CoreImpl::setSystem(System *system) {
    this->system = system;
}

void CoreImpl::loop() {


    if (!persistent->is_mqttsn_online() || !persistent->is_mqtt_online()) {
        if (persistent->is_mqttsn_online() && !persistent->is_mqtt_online()) {
            // only mqttsn is online => disconnect all connected client
            process_mqtt_offline_procedure();
            return;
        } else if (!persistent->is_mqttsn_online() && persistent->is_mqtt_online()) {
            // only mqtt is online => publish will-message and disconnect
            process_mqttsn_offline_procedure();
            set_all_clients_lost();
            return;
        } else {
            // both are disconnected
            set_all_clients_lost();
            system->exit();
        }
    }

    bool has_heart_beaten = system->has_beaten();

    // TODO write advertise here
    // if hasbeaten and > advertise duration and core->mqtt/sn connecte) => send advertise package
#if CORE_DEBUG
    if (has_heart_beaten) {
        logger->start_log("check client for timeout", 3);
    }
#endif

    // create buckets for to the client data
    device_address last_client_address;
    memset(&last_client_address, 0, sizeof(device_address));

    char client_id[24];
    device_address address;
    CLIENT_STATUS status;
    uint32_t timeout;
    uint32_t duration;

    uint32_t elapsed_time = 0;
    if (has_heart_beaten) {
        elapsed_time = system->get_elapsed_time();
    }
    persistent->get_last_client_address(&last_client_address);

    bool is_last_client_address_empty = true;
    for (uint8_t i = 0; i < sizeof(device_address); i++) {
        if (last_client_address.bytes[i] != 0) {
            is_last_client_address_empty = false;
            break;
        }
    }
    if (is_last_client_address_empty) {
#if CORE_DEBUG
        logger->log("no clients connected", 3);
#endif
        return;
    }

    persistent->get_nth_client(0, client_id, &address, &status, &duration, &timeout);
    uint64_t i = 1;
    do {
        handle_client_publishes(status, client_id, &address);
        persistent->start_client_transaction(&address);
        if (status == AWAKE && !persistent->has_client_publishes()) {
            persistent->set_client_state(ASLEEP);
            uint8_t transaction_return = persistent->apply_transaction();
            if (transaction_return == SUCCESS) {
                mqttsn->send_pingresp(&address);
            }
        } else {
            uint8_t transaction_return = persistent->apply_transaction();
        }
        if (has_heart_beaten) {
            handle_timeout(status, duration, elapsed_time, client_id, address, timeout);
        }
        persistent->get_nth_client(i++, client_id, &address, &status, &duration, &timeout);
        bool is_address_empty = true;
        for (uint8_t i = 0; i < sizeof(device_address); i++) {
            if (address.bytes[i] != 0) {
                is_address_empty = false;
                break;
            }
        }
        if (is_address_empty) {
            return;
        }
    } while (memcmp(&last_client_address, &address, sizeof(device_address)) != 0);
    handle_client_publishes(status, client_id, &address);
    persistent->start_client_transaction(&address);
    if (status == AWAKE && !persistent->has_client_publishes()) {
        persistent->set_client_state(ASLEEP);
        uint8_t transaction_return = persistent->apply_transaction();
        if (transaction_return == SUCCESS) {
            mqttsn->send_pingresp(&address);
        }
    } else {
        uint8_t transaction_return = persistent->apply_transaction();
    }
    if (has_heart_beaten) {
        handle_timeout(status, duration, elapsed_time, client_id, address, timeout);
    }
}

void
CoreImpl::handle_timeout(const CLIENT_STATUS &status, uint32_t duration, uint32_t elapsed_time, char *client_id,
                         device_address &address,
                         uint32_t &timeout) {
    if (status == ACTIVE || status == ASLEEP || status == AWAKE) {
        persistent->start_client_transaction(&address);
        timeout += elapsed_time;
        uint64_t tolerance_timeout = 0;
        if (duration > 60000) { // greater or smaller 60 seconds
            // +10%
            tolerance_timeout = (uint64_t) (duration + (duration / 10.0));
        } else {
            // +50%
            tolerance_timeout = (uint64_t) (duration + (duration / 2.0));
        }
        persistent->set_timeout(timeout);
#if CORE_DEBUG
        char buf[24];
        memset(&buf, 0, sizeof(buf));
        persistent->get_client_id((char *) &buf);
        logger->start_log(buf, 1);
        logger->append_log(" ");
        append_device_address(&address);
        logger->append_log(" seconds since last message: ");
        sprintf(buf, "%d", timeout);
        logger->append_log(buf);
#endif
        if (timeout > tolerance_timeout) {
            persistent->set_client_state(LOST);
#if CORE_LOG
            logger->start_log("Client ", 1);
            char client_id_buf[24];
            char uint16_buf[20];
            memset(&client_id_buf, 0, sizeof(client_id_buf));
            persistent->get_client_id((char *) &client_id_buf);
            logger->append_log(" (d");
            sprintf(uint16_buf, "%d", duration);
            logger->append_log(uint16_buf);
            logger->append_log(", t");
            sprintf(uint16_buf, "%d", timeout);
            logger->append_log(uint16_buf);
            logger->append_log(")");
            logger->append_log(" has exceeded timeout, lost");
#endif

            if (persistent->has_client_will()) {
#if CORE_LOG
                logger->append_log(" - will");
#endif
                char willtopic[255];
                memset(&willtopic, 0, sizeof(willtopic));
                uint8_t willmsg[255];
                memset(&willmsg, 0, sizeof(willmsg));
                uint8_t willmsg_length = 0;
                uint8_t qos = 0;
                bool retain = false;
                persistent->get_client_will((char *) willtopic, (uint8_t *) &willmsg, &willmsg_length, &qos,
                                            &retain);

#if CORE_LOG
                logger->append_log(" (q");
                sprintf(uint16_buf, "%d", qos);
                logger->append_log(uint16_buf);
                logger->append_log(", r");
                logger->append_log(retain ? "1" : "0");
                logger->append_log(", ... (");
                sprintf(uint16_buf, "%d", willmsg_length);
                logger->append_log(uint16_buf);
                logger->append_log(" bytes), '");
                logger->append_log(willtopic);
                logger->append_log("')'");
#endif

                // NOT_SUPPORTED: will message retry
                bool will_publish_result = mqtt->publish(willtopic, (const uint8_t *) willmsg, willmsg_length, qos,
                                                         retain);
#if CORE_LOG
                if (will_publish_result) {
                    logger->append_log(" - SEND");
                } else {
                    logger->append_log(" - SEND FAILURE");
                }
#endif
                remove_client_subscriptions(client_id);
                persistent->delete_client(client_id);
            } else {
                logger->append_log(" - no will");
            }
        }
        persistent->apply_transaction();
    }
}


CORE_RESULT
CoreImpl::add_client(const char *client_id, uint16_t duration, bool clean_session, device_address *address) {
#if CORE_LOG
    char uint16_buffer[20];
    logger->start_log("New client connected", 1);
    logger->append_log(" from ");
    append_device_address(address);
    logger->append_log(" as ");
    logger->append_log(client_id);
    logger->append_log(" (c");
    logger->append_log(clean_session ? "1" : "0");
    logger->append_log(", d");
    sprintf(uint16_buffer, "%d", duration);
    logger->append_log(uint16_buffer);
    logger->append_log(")");
#endif
    if (!persistent->is_mqtt_online()) {
#if CORE_LOG
        logger->append_log(" - REJECTED MQTT OFFLINE");
#endif
        return ZERO;
    }
    persistent->start_client_transaction(client_id);
    if (persistent->client_exist()) {
        if (!clean_session) {
            persistent->reset_client(client_id, address, ((uint32_t) duration) * 1000);
        } else {

            // before we can remove the client, we must unsubscribe from all subscriptions
            this->remove_client_subscriptions(client_id);
            persistent->delete_client(client_id);
            persistent->add_client(client_id, address, ((uint32_t) duration) * 1000);
        }
    } else {
        persistent->add_client(client_id, address, ((uint32_t) duration) * 1000);
    }

    uint8_t result = persistent->apply_transaction();
    if (result == SUCCESS) {
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(" - SUCCESS");
#endif
        return SUCCESS;
    } else if (result == FULL) {
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(" - REJECTED FULL");
#endif
        return FULL;
    }
#if CORE_LOG
    logger->set_current_log_lvl(1);
    logger->append_log(" - REJECTED");
#endif
    return ZERO;
}

CORE_RESULT CoreImpl::remove_will(device_address *address) {
    persistent->start_client_transaction(address);
    persistent->delete_will();
    uint8_t result = persistent->apply_transaction();
    if (result == SUCCESS) {
        return SUCCESS;
    } else if (result == FULL) {
        return FULL;
    }
    return ZERO;
}

CORE_RESULT CoreImpl::add_will_topic(device_address *address, char *will_topic, bool retain, uint8_t qos) {
    if (!persistent->is_mqtt_online()) {
        return ZERO;
    }
    persistent->start_client_transaction(address);
    if (persistent->get_client_await_message_type() != MQTTSN_WILLTOPIC ||
        persistent->get_client_await_message_type() == MQTTSN_PINGREQ) {
#if CORE_DEBUG
        logger->log("NOT WAITING FOR WILLTOPIC", 3);
#endif
        persistent->apply_transaction();
        return ZERO;
    }

    persistent->set_client_willtopic(will_topic, qos, retain);

    uint8_t result = persistent->apply_transaction();
    if (result == SUCCESS) {
        return SUCCESS;
    } else if (result == FULL) {
        return FULL;
    }
    return ZERO;
}

CORE_RESULT CoreImpl::add_will_msg(device_address *address, uint8_t *will_msg, uint8_t will_msg_len) {
    if (!persistent->is_mqtt_online()) {
        return ZERO;
    }
    persistent->start_client_transaction(address);
    if (persistent->get_client_await_message_type() != MQTTSN_WILLMSG ||
        persistent->get_client_await_message_type() == MQTTSN_PINGREQ) {
#if CORE_DEBUG
        logger->log("NOT WAITING FOR WILLMSG", 3);
#endif
        persistent->apply_transaction();
        return ZERO;
    }
    persistent->set_client_willmessage(will_msg, will_msg_len);

    uint8_t result = persistent->apply_transaction();
    if (result == SUCCESS) {
        return SUCCESS;
    } else if (result == FULL) {
        return FULL;
    }
    return ZERO;
}

CORE_RESULT CoreImpl::await_message(device_address *address, message_type type) {
    persistent->start_client_transaction(address);
    persistent->set_client_await_message(type);
    uint8_t result = persistent->apply_transaction();

    if (result == SUCCESS) {
        return SUCCESS;
    } else if (result == FULL) {
        return FULL;
    }
    return ZERO;
}

CORE_RESULT CoreImpl::register_topic(device_address *address, char *topic_name, uint16_t *new_topic_id) {
#if CORE_LOG
    logger->start_log("Received REGISTER", 1);
#endif
    if (!persistent->is_mqtt_online()) {
#if CORE_LOG
        logger->append_log(" - REJECTED MQTT OFFLINE");
#endif
        return ZERO;
    }

    persistent->start_client_transaction(address);
#if CORE_LOG
    if (persistent->client_exist()) {
        logger->set_current_log_lvl(1);
        logger->append_log(" from ");
        char client_id[24];
        memset(client_id, 0, sizeof(client_id));
        persistent->get_client_id(client_id);
        logger->append_log(client_id);
    } else {
        logger->set_current_log_lvl(1);
        append_device_address(address);
    }
    char uint16_buffer[20];
    logger->append_log(" (");
    logger->append_log("'");
    logger->append_log(topic_name);
    logger->append_log("'");
#endif

    persistent->add_client_registration(topic_name, new_topic_id);
    uint8_t result = persistent->apply_transaction();
    if (result == FULL) {
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(") - FULL");
#endif
        return FULL;
    } else if (result == CLIENTNONEXISTENCE) {
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(") - ClIENT NOT CONNECTED");
#endif
        return CLIENTNONEXISTENCE;
    }

    if (result == SUCCESS) {
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(", ");
        logger->append_log("t");
        sprintf(uint16_buffer, "%d", *new_topic_id);
        logger->append_log(uint16_buffer);
        logger->append_log(") - SUCCESS");
#endif
        return SUCCESS;
    }
#if CORE_LOG
    logger->set_current_log_lvl(1);
    logger->append_log(") - ClIENT NOT CONNECTED");
#endif
    return ZERO;
}


CORE_RESULT
CoreImpl::publish(device_address *address, const uint8_t *data, uint16_t data_len, uint16_t msg_id,
                  uint16_t topic_id,
                  bool short_topic, bool retain, int8_t qos, bool dup, uint16_t *new_topic_id) {
#if CORE_LOG
    logger->start_log("Received PUBLISH", 1);
#endif
    if (!persistent->is_mqtt_online()) {
#if CORE_LOG
        logger->append_log(" - REJECTED MQTT OFFLINE");
#endif
        return ZERO;
    }
    persistent->start_client_transaction(address);

#if CORE_LOG
    if (persistent->client_exist()) {
        logger->set_current_log_lvl(1);
        logger->append_log(" from ");
        char client_id[24];
        memset(client_id, 0, sizeof(client_id));
        persistent->get_client_id(client_id);
        logger->append_log(client_id);
    } else {
        logger->set_current_log_lvl(1);
        append_device_address(address);
    }
    char uint16_buffer[20];
    logger->append_log(" (q");
    sprintf(uint16_buffer, "%d", qos);
    logger->append_log(uint16_buffer);
    logger->append_log(", p");
    logger->append_log(short_topic ? "0" : "1");
    logger->append_log(", r");
    logger->append_log(retain ? "1" : "0");
    logger->append_log(", d");
    logger->append_log(dup ? "1" : "0");
    logger->append_log(", t");
    sprintf(uint16_buffer, "%d", topic_id);
    logger->append_log(uint16_buffer);
    logger->append_log(", m");
    sprintf(uint16_buffer, "%d", msg_id);
    logger->append_log(uint16_buffer);
    logger->append_log(", ...(");
    sprintf(uint16_buffer, "%d", data_len);
    logger->append_log(uint16_buffer);
    logger->append_log(" bytes)");
#endif

    if (!persistent->client_exist() ||
        (persistent->client_exist() &&
         (persistent->get_client_status() == DISCONNECTED ||
          persistent->get_client_status() == LOST))) {
        // client does not exists
        // or client is not in any connection state
        if (qos == -1 && !short_topic) {
            char *topic_name = persistent->get_predefined_topic_name(topic_id);
            if (topic_name != nullptr) {
#if CORE_LOG
                logger->append_log(", '");
                logger->append_log(topic_name);
                logger->append_log("')");
#endif
                qos = 0;
                bool mqtt_result = mqtt->publish(topic_name, data, data_len, (uint8_t) qos, retain);
                uint8_t result = persistent->apply_transaction();
                if (mqtt_result && result == SUCCESS) {
#if CORE_LOG
                    logger->append_log(" - SEND");
#endif
                    return SUCCESS;
                }
#if CORE_LOG
                logger->append_log(" - SEND FAILURE");
#endif
                return ZERO;
            }
#if CORE_LOG
            if (topic_name == nullptr) {
                logger->append_log(", unknown topic id)");
                logger->append_log(" - FAILURE");
            }
#endif
        }
        uint8_t result = persistent->apply_transaction();
#if CORE_LOG
        logger->append_log(")");
#endif
        if (result == SUCCESS) {
#if CORE_LOG
            logger->append_log(" - NOT CONNECTED CLIENT PUBLISHES QOS M1 TO NOT PREDEFINED TOPIC ID");
#endif
            return CLIENTNONEXISTENCE;
        }
#if CORE_LOG
        logger->append_log(" - INTERNAL ERROR");
#endif
        return ZERO;
    }

    const char *topic_name = nullptr;
    if (short_topic) {
        topic_name = persistent->get_topic_name(topic_id);
    } else {
        topic_name = persistent->get_predefined_topic_name(topic_id);
    }
    if (topic_name == nullptr) {
        uint8_t result = persistent->apply_transaction();
#if CORE_LOG
        logger->append_log(", unknown topic id)");
        logger->append_log(" - FAILURE");
#endif
        if (result == SUCCESS) {
            return TOPICIDNONEXISTENCE;
        }
        return ZERO;
    }
#if CORE_LOG
    logger->append_log(", '");
    logger->append_log(topic_name);
    logger->append_log("')");
#endif
    uint8_t result = persistent->apply_transaction();
    if (result != SUCCESS) {
#if CORE_LOG
        logger->append_log(" - INTERNAL ERROR");
#endif
        return ZERO;
    }

    if (qos == -1) {
        qos = 0;
    }
    bool mqtt_result = mqtt->publish(topic_name, data, data_len, (uint8_t) qos, retain);

    if (!mqtt_result) {
#if CORE_LOG
        logger->append_log(" - SEND FAILURE");
#endif
        return ZERO;
    }
#if CORE_LOG
    logger->append_log(" - SEND");
#endif

    return SUCCESS;
}

CORE_RESULT
CoreImpl::add_subscription(device_address *address, uint16_t topic_id, uint16_t msg_id, bool short_topic,
                           uint8_t qos, bool dup, uint16_t *new_topic_id, uint8_t *granted_qos) {
#if CORE_LOG
    logger->start_log("Received SUBSCRIBE", 1);
#endif
    if (!persistent->is_mqtt_online()) {
#if CORE_LOG
        logger->append_log(" - REJECTED MQTT OFFLINE");
#endif
        return ZERO;
    }

    persistent->start_client_transaction(address);

#if CORE_LOG
    if (persistent->client_exist()) {
        logger->set_current_log_lvl(1);
        logger->append_log(" from ");
        char client_id[24];
        memset(client_id, 0, sizeof(client_id));
        persistent->get_client_id(client_id);
        logger->append_log(client_id);
    } else {
        logger->set_current_log_lvl(1);
        append_device_address(address);
    }
    char uint16_buffer[20];
    logger->append_log(" (q");
    sprintf(uint16_buffer, "%d", qos);
    logger->append_log(uint16_buffer);
    logger->append_log(", p");
    logger->append_log(short_topic ? "0" : "1");
    logger->append_log(", d");
    logger->append_log(dup ? "1" : "0");
    logger->append_log(", t");
    sprintf(uint16_buffer, "%d", topic_id);
    logger->append_log(uint16_buffer);
    logger->append_log(", m");
    sprintf(uint16_buffer, "%d", msg_id);
    logger->append_log(uint16_buffer);
#endif

    if (!persistent->client_exist()) {
        uint8_t result = persistent->apply_transaction();
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(") - CLIENT NOT CONNECTED");
#endif
        return ZERO;
    }

    bool subscribe = false;
    *granted_qos = 0;
    if (qos >= 1) {
        *granted_qos = 1;
    }
    // NOT_SUPPORTED: at the moment we only grant a maximum qos subscription of 1

    const char *topic_name = nullptr;
    if (short_topic) {
        topic_name = persistent->get_topic_name(topic_id);
        if (topic_name != nullptr) {
            *new_topic_id = topic_id;
        }
    } else {
        topic_name = persistent->get_predefined_topic_name(topic_id);
        if (topic_name != nullptr) {
            persistent->add_client_registration((char *) topic_name, new_topic_id);
        }
    }

    if (topic_name == nullptr) {
        uint8_t result = persistent->apply_transaction();
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(") - TOPIC ID UNKNOWN");
#endif
        return ZERO;
    }

#if CORE_LOG
    logger->append_log(", '");
    logger->append_log(topic_name);
    logger->append_log("', t");
    sprintf(uint16_buffer, "%d", *new_topic_id);
    logger->append_log(uint16_buffer);
#endif


    if (!persistent->is_subscribed(topic_name)) {
        persistent->add_subscription(topic_name, *new_topic_id, *granted_qos);
        persistent->increment_global_subscription_count(topic_name);
        uint32_t subscription_count = persistent->get_global_topic_subscription_count(topic_name);
#if CORE_LOG
        logger->append_log(", c");
        sprintf(uint16_buffer, "%d", subscription_count);
        logger->append_log(uint16_buffer);
#endif
        if (subscription_count == 1) { // we are the first to subscribe to this topic.
            subscribe = true;
        }
    } else {
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(") - ALREADY SUBSCRIBED");
#endif
    }
    // else client has already a subscription to the topic

    uint8_t result = persistent->apply_transaction();

    if (result == FULL) {
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(") - FULL");
#endif
        return FULL;
    } else if (result == CLIENTNONEXISTENCE) {
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(") - CLIENT NOT CONNECTED");
#endif
        return CLIENTNONEXISTENCE;
    } else if (result == CLIENTSUBSCRIPTIONFULL) {
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(") - CLIENT SUBSCRIPTION FULL");
#endif
        return CLIENTSUBSCRIPTIONFULL;
    } else if (result == TOPICIDNONEXISTENCE) {
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(") - TOPIC ID UNKNOWN");
#endif
        return TOPICIDNONEXISTENCE;
    }

    if (subscribe) {
        if (!mqtt->subscribe(topic_name, 1)) {
            persistent->decrement_global_subscription_count(topic_name);
#if CORE_LOG
            logger->set_current_log_lvl(1);
            logger->append_log(") - SUBSCRIPTION FAILED");
#endif
            return ZERO;
        }
    }

    if (result == SUCCESS) {
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(") - SUCCESS");
#endif
        return SUCCESS;
    }
#if CORE_LOG
    logger->set_current_log_lvl(1);
    logger->append_log(") - INTERNAL ERROR");
#endif
    return ZERO;
}

CORE_RESULT
CoreImpl::delete_subscription(device_address *address, uint16_t topic_id, uint16_t msg_id, bool short_topic,
                              bool dup) {
    persistent->start_client_transaction(address);
    const char *topic_name = nullptr;
    if (short_topic) {
        topic_name = persistent->get_topic_name(topic_id);
    } else {
        topic_name = persistent->get_predefined_topic_name(topic_id);
    }

    bool unsubscribe = false;
    if (!persistent->is_subscribed(topic_name)) {
        persistent->delete_subscription(topic_id);
        persistent->decrement_global_subscription_count(topic_name);
        uint32_t subscription_count = persistent->get_global_topic_subscription_count(topic_name);
        if (subscription_count == 0) { // we are the first to subscribe to this topic.
            unsubscribe = true;
        }
    }

    uint8_t result = persistent->apply_transaction();
    if (result == FULL) {
        return FULL;
    } else if (result == CLIENTNONEXISTENCE) {
        return CLIENTNONEXISTENCE;
    } else if (result == TOPICIDNONEXISTENCE) {
        return TOPICIDNONEXISTENCE;
    }
    if (unsubscribe) {
        if (!mqtt->unsubscribe(topic_name)) {
            return ZERO;
        }
    }

    if (result == SUCCESS) {
        return SUCCESS;
    }
    return ZERO;
}

CORE_RESULT CoreImpl::delete_subscription(device_address *address, char *topic_name, uint16_t msg_id, bool dup) {
    persistent->start_client_transaction(address);
    uint16_t topic_id = persistent->get_topic_id(topic_name);

    bool unsubscribe = false;
    if (!persistent->is_subscribed(topic_name)) {
        persistent->delete_subscription(topic_id);
        persistent->decrement_global_subscription_count(topic_name);
        uint32_t subscription_count = persistent->get_global_topic_subscription_count(topic_name);
        if (subscription_count == 0) { // we are the last to unsubscribe to this topic.
            unsubscribe = true;
        }
    }

    uint8_t result = persistent->apply_transaction();
    if (result == FULL) {
        return FULL;
    } else if (result == CLIENTNONEXISTENCE) {
        return CLIENTNONEXISTENCE;
    } else if (result == TOPICIDNONEXISTENCE) {
        return TOPICIDNONEXISTENCE;
    }
    if (unsubscribe) {
        if (!mqtt->unsubscribe(topic_name)) {
            return ZERO;
        }
    }

    if (result == SUCCESS) {
        return SUCCESS;
    }
    return ZERO;
}

CORE_RESULT CoreImpl::set_disconnected(device_address *address) {
    persistent->start_client_transaction(address);

#if CORE_LOG
    logger->start_log("Received DISCONNECT", 1);
    if (persistent->client_exist()) {
        logger->set_current_log_lvl(1);
        logger->append_log(" from ");
        char client_id[24];
        memset(client_id, 0, sizeof(client_id));
        persistent->get_client_id(client_id);
        logger->append_log(client_id);
    } else {
        logger->set_current_log_lvl(1);
        append_device_address(address);
    }
#endif

    persistent->set_client_state(DISCONNECTED);
    uint8_t result = persistent->apply_transaction();
    if (result == CLIENTNONEXISTENCE) {
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(" - CLIENT NOT CONNECTED");
#endif
        return CLIENTNONEXISTENCE;
    }

    if (result == SUCCESS) {
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(" - SUCCESS");
#endif
        return SUCCESS;
    }
#if CORE_LOG
    logger->set_current_log_lvl(1);
    logger->append_log(" - INTERNAL ERROR");
#endif
    return ZERO;
}

CORE_RESULT CoreImpl::set_asleep(device_address *address, uint16_t duration) {
#if CORE_LOG
    logger->start_log("Received DISCONNECT", 1);
    if (persistent->client_exist()) {
        logger->set_current_log_lvl(1);
        logger->append_log(" from ");
        char client_id[24];
        memset(client_id, 0, sizeof(client_id));
        persistent->get_client_id(client_id);
        logger->append_log(client_id);
    } else {
        logger->set_current_log_lvl(1);
        append_device_address(address);
    }
    char uint16_buf[20];
    sprintf(uint16_buf, "%d", duration);
    logger->append_log(" (d");
    logger->append_log(uint16_buf);
    logger->append_log(")");
#endif

    if (!persistent->is_mqtt_online()) {
#if CORE_LOG
        logger->append_log(" - REJECTED MQTT OFFLINE");
#endif
        return ZERO;
    }
    persistent->start_client_transaction(address);
    persistent->set_client_state(ASLEEP);
    persistent->set_client_duration(((uint32_t) duration) * 1000); // save it in milliseconds

    uint8_t result = persistent->apply_transaction();
    if (result == CLIENTNONEXISTENCE) {
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(" - CLIENT NOT CONNECTED");
#endif
        return CLIENTNONEXISTENCE;
    }

    if (result == SUCCESS) {
#if CORE_LOG
        logger->set_current_log_lvl(1);
        logger->append_log(" - SUCCESS");
#endif
        return SUCCESS;
    }
#if CORE_LOG
    logger->set_current_log_lvl(1);
    logger->append_log(" - INTERNAL ERROR");
#endif
    return ZERO;
}

CORE_RESULT CoreImpl::set_asleep(device_address *address) { // TODO NEVER USED REMOVE!

    if (!persistent->is_mqtt_online()) {
        return ZERO;
    }
    persistent->start_client_transaction(address);
    persistent->set_client_state(ASLEEP);

    uint8_t result = persistent->apply_transaction();
    if (result == CLIENTNONEXISTENCE) {
        return CLIENTNONEXISTENCE;
    }

    if (result == SUCCESS) {
        return SUCCESS;
    }
    return ZERO;
}


CORE_RESULT CoreImpl::notify_regack_arrived(device_address *address, uint16_t topic_id, uint16_t msg_id,
                                            return_code_t return_code) {
    persistent->start_client_transaction(address);
    if (persistent->get_client_await_message_type() == MQTTSN_REGACK &&
        persistent->get_client_await_msg_id() == msg_id) {
        if (return_code == ACCEPTED) {
            persistent->set_topic_known(topic_id, true);
            persistent->set_client_await_message(MQTTSN_PINGREQ);
        } else if (return_code == REJECTED_CONGESTION) {
            persistent->set_topic_known(topic_id, false);
            persistent->set_client_await_message(MQTTSN_PINGREQ);
        } else if (return_code == REJECTED_INVALID_TOPIC_ID) {
            // register topic to client again, but first fix subscriptions and publishes
            // this is very very big inconsistency: implement later
            // NOT_SUPPORTED: implement me later
            // NOT_SUPPORTED delete subscriptions, publishes and registrations with the topic_id
#if CORE_DEBUG
            logger->log("FATAL ERROR: regack return_code: REJECTED_INVALID_TOPIC_ID ", 0);
#endif
            persistent->set_topic_known(topic_id, true);
            persistent->apply_transaction();
            return ZERO;
        } else if (return_code == REJECTED_NOT_SUPPORTED) {
            // NOT_SUPPORTED delete subscriptions, publishes and registrations with the topic_id
#if CORE_DEBUG
            logger->log("FATAL ERROR: regack return_code: REJECTED_NOT_SUPPORTED ", 0);
#endif
            persistent->set_topic_known(topic_id, true);
            persistent->apply_transaction();
            return ZERO;
        } else {
            // unkown return_code, return ZERO
            persistent->apply_transaction();
            return ZERO;
        }

    }
    uint8_t result = persistent->apply_transaction();
    if (result == CLIENTNONEXISTENCE) {
        return CLIENTNONEXISTENCE;
    }

    if (result == SUCCESS) {
        return SUCCESS;
    }
}

CORE_RESULT CoreImpl::notify_puback_arrived(device_address *address, uint16_t msg_id, uint16_t topic_id,
                                            return_code_t return_code) {
    persistent->start_client_transaction(address);
    if (return_code == ACCEPTED) {
        persistent->remove_publish_by_msg_id(msg_id);
    } else if (return_code == REJECTED_CONGESTION) {
        // try again later, but first remove msg_id and set dup flag
        //persistent->reset_client_publish_msg_id(msg_id);
        //persistent->set_client_publish_dub_flag(msg_id);
        // NOT_SUPPORTED: implement me later
        persistent->remove_publish_by_msg_id(msg_id);
    } else if (return_code == REJECTED_INVALID_TOPIC_ID) {
        // register topic to client again, but first fix subscriptions and publishes
        // this is very very big inconsistency: implement later
        // NOT_SUPPORTED: implement me later
        persistent->remove_publish_by_msg_id(msg_id);
    } else if (return_code == REJECTED_NOT_SUPPORTED) {
        // this means: client cannot receive publishes, or client cannot receive qos 1/2 publishes
        // do: subscription_qos = subscription_qos - 1;
        // NOT_SUPPORTED: implement me later
        persistent->remove_publish_by_msg_id(msg_id);
    } else {
        // unkown return_code, return ZERO
        persistent->apply_transaction();
        return ZERO;
    }

    uint8_t result = persistent->apply_transaction();
    if (result == CLIENTNONEXISTENCE) {
        return CLIENTNONEXISTENCE;
    }

    if (result == SUCCESS) {
        return SUCCESS;
    }
    return ZERO;
}


CORE_RESULT CoreImpl::set_awake(device_address *address) {
    persistent->start_client_transaction(address);
    persistent->set_client_state(AWAKE);

    uint8_t result = persistent->apply_transaction();
    if (result == CLIENTNONEXISTENCE) {
        return CLIENTNONEXISTENCE;
    }

    if (result == SUCCESS) {
        return SUCCESS;
    }
    return ZERO;
}

CORE_RESULT CoreImpl::get_gateway_id(uint8_t *gateway_id) {
    if (persistent->get_gateway_id(gateway_id)) {
        return SUCCESS;
    }
    return ZERO;
}

CORE_RESULT CoreImpl::notify_mqttsn_disconnected() {
    uint8_t result = persistent->set_mqttsn_disconnected();
    if (result == SUCCESS) {
        return SUCCESS;
    }
    return ZERO;
}

CORE_RESULT CoreImpl::notify_mqtt_disconnected() {
    uint8_t result = persistent->set_mqtt_disconnected();
    if (result == SUCCESS) {
        return SUCCESS;
    }
    return ZERO;
}

CORE_RESULT CoreImpl::publish(char *topic_name, uint8_t *data, uint32_t data_length, bool retain) {
#if CORE_LOG
    logger->start_log("Received PUBLISH from Broker ", 1);
    logger->append_log("(r");
    logger->append_log(retain ? "1" : "0");
    logger->append_log(", ... (");
    char uint16_buf[20];
    sprintf(uint16_buf, "%d", data_length);
    logger->append_log(uint16_buf);
    logger->append_log(" bytes)");
    logger->append_log(", '");
    logger->append_log(topic_name);
    logger->append_log("')");
#endif
    if (data_length > mqttsn->get_maximum_publish_payload_length()) {
#if CORE_LOG
        logger->append_log(" - TOO MUCH PAYLOAD");
#endif
        return SUCCESS;
    }
    uint32_t subscription_count = persistent->get_global_topic_subscription_count(topic_name);
#if CORE_LOG
    logger->set_current_log_lvl(1);
    logger->append_log(" | (c");
    sprintf(uint16_buf, "%d", subscription_count);
    logger->append_log(uint16_buf);
    logger->append_log(")");
#endif

    // TODO implement message saving
#if CORE_LOG
    logger->append_log(" - MESSAGE SAVING IS EXPERIMENTAL");
#endif
    //return SUCCESS;

    // create buckets for to the client data
    device_address last_client_address;
    memset(&last_client_address, 0, sizeof(device_address));

    char client_id[24];
    device_address address;
    CLIENT_STATUS status;
    uint32_t timeout;
    uint32_t duration;

    persistent->get_last_client_address(&last_client_address);

    bool is_last_client_address_empty = true;
    for (uint8_t i = 0; i < sizeof(device_address); i++) {
        if (last_client_address.bytes[i] != 0) {
            is_last_client_address_empty = false;
            break;
        }
    }
    if (is_last_client_address_empty) {
#if CORE_DEBUG
        logger->append_log(" - no clients connected");
#endif
        return SUCCESS;
    }
    // TODO fix loop (endless) // later
    persistent->get_nth_client(0, client_id, &address, &status, &duration, &timeout);
    uint64_t i = 1;
    do {
        handle_receive_mqtt_publish_for_client(topic_name, data, data_length, address, retain);
        persistent->get_nth_client(i++, client_id, &address, &status, &duration, &timeout);

        bool is_address_empty = true;
        for (uint8_t i = 0; i < sizeof(device_address); i++) {
            if (address.bytes[i] != 0) {
                is_address_empty = false;
                break;
            }
        }
        if (is_address_empty) {
            break;
        }

    } while (memcmp(&last_client_address, &address, sizeof(device_address)) != 0);

    return SUCCESS;
}

void CoreImpl::handle_receive_mqtt_publish_for_client(const char *topic_name, uint8_t *data, uint32_t data_length,
                                                      device_address &address,
                                                      bool retain) {
    uint8_t transaction_return;
    persistent->start_client_transaction(&address);
    if ((persistent->get_client_status() == ACTIVE ||
         (persistent->get_client_status() == ASLEEP || persistent->get_client_status() == AWAKE))
        && persistent->is_subscribed(topic_name)) {
        int8_t qos = persistent->get_subscription_qos(topic_name);
        uint16_t topic_id = persistent->get_subscription_topic_id(topic_name);

        // message are saved first, then processed during loop in handle_client_publish
        persistent->add_new_client_publish(data, (uint8_t) data_length, topic_id, retain, (uint8_t) qos);

    }
    // we cannot do anything with the return value, except logging
    transaction_return = persistent->apply_transaction();
#if CORE_DEBUG
    if (transaction_return != SUCCESS) {
        logger->start_log(" - error", 3);
        return;
    }
    logger->start_log(" - success", 3);
#endif
}

CORE_RESULT CoreImpl::get_mqtt_config(uint8_t *server_ip, uint16_t *server_port, char *client_id) {
    if (persistent->get_mqtt_config(server_ip, server_port, client_id)) {
        return SUCCESS;
    }
    return ZERO;
}

CORE_RESULT CoreImpl::get_mqtt_login_config(char *username, char *password) {
    if (persistent->get_mqtt_login_config(username, password)) {
        return SUCCESS;
    }
    return ZERO;
}

CORE_RESULT CoreImpl::get_mqtt_will(char *will_topic, char *will_msg, uint8_t *will_qos, bool *will_retain) {
    if (persistent->get_mqtt_will(will_topic, will_msg, will_qos, will_retain)) {
        return SUCCESS;
    }
    return ZERO;
}

CORE_RESULT CoreImpl::notify_mqtt_connected() {
    uint8_t result = persistent->set_mqtt_connected();
    if (result == SUCCESS) {
        return SUCCESS;
    }
    return ZERO;
}

CORE_RESULT CoreImpl::notify_mqttsn_connected() {
    uint8_t result = persistent->set_mqttsn_connected();
    if (result == SUCCESS) {
        return SUCCESS;
    }
    return ZERO;
}

void CoreImpl::remove_client_subscriptions(const char *client_id) {
#if CORE_DEBUG
    logger->start_log("remove client subscription ", 3);
    logger->append_log(client_id);

#endif
    uint16_t client_subscription_count = 0;
    while ((client_subscription_count = persistent->get_client_subscription_count()) > 0) {
#if CORE_DEBUG
        char uint16_buf[20];
        sprintf(uint16_buf, "%d", client_subscription_count);
        logger->start_log(" client_subscription_count ", 3);
        logger->append_log(uint16_buf);
#endif
        uint16_t topic_id = persistent->get_nth_subscribed_topic_id(client_subscription_count - (uint16_t) 1);
        const char *topic_name = persistent->get_topic_name(topic_id);

#if CORE_DEBUG
//        char uint16_buf[20];
        sprintf(uint16_buf, "%d", topic_id);
        logger->start_log("topic_id ", 4);
        logger->append_log(uint16_buf);
        logger->append_log(" topic_name");
        logger->append_log(topic_name);
#endif
        persistent->delete_subscription(topic_id);
        persistent->decrement_global_subscription_count(topic_name);
        uint32_t subscription_count = persistent->get_global_topic_subscription_count(topic_name);
#if CORE_DEBUG
        logger->start_log(" subscriptin count ", 3);
        sprintf(uint16_buf, "%d", subscription_count);
        logger->append_log(uint16_buf);
#endif
        if (subscription_count == 0) {
#if CORE_DEBUG
            logger->log("unsusbcribe topic", 3);
#endif
            mqtt->unsubscribe(topic_name);
        }
    }
}

void CoreImpl::process_mqttsn_offline_procedure() {
    char will_topic[255];
    memset(&will_topic, 0, sizeof(will_topic));
    char will_msg[255];
    memset(&will_msg, 0, sizeof(will_msg));
    uint8_t will_qos = 0;
    bool will_retain = false;
    bool has_mqtt_will = get_mqtt_will(will_topic, will_msg, &will_qos, &will_retain);
    if (has_mqtt_will) {
        // we ignore if it works or not atm
        mqtt->publish(will_topic, (const uint8_t *) will_msg, (uint16_t) (strlen(will_msg) + 1), will_qos,
                      will_retain);
    }
    mqtt->disconnect();

    system->sleep(5000);
    system->exit();
}

void CoreImpl::process_mqtt_offline_procedure() {
    // the real process mqtt offline procedure is performed in the different core function
    // only accepted messages are:
    // - MQTTSN_PINGREQ with client id field
    // - MQTTSN_DISCONNECT without duration field
    // - MQTTSN_REGACK
    // - MQTTSN_PUBACK
    // Because we perform the publish of message in the loop of the core-class.
    // We keep looping until.

    // in this method we check if a client has more publishes, if not we disconnect him.
    // afterwards we check if the we have connected clients, if not we shutdown the gateway.


    bool has_heart_beaten = system->has_beaten();

#if CORE_DEBUG
    if (has_heart_beaten) {
        logger->start_log("check client for timeout", 3);
    }
#endif


    // create buckets for to the client data
    device_address last_client_address;
    memset(&last_client_address, 0, sizeof(device_address));

    char client_id[24];
    device_address address;
    CLIENT_STATUS status;
    uint32_t timeout;
    uint32_t duration;

    uint32_t elapsed_time = system->get_elapsed_time() / 1000;

    persistent->get_last_client_address(&last_client_address);

    bool is_last_client_address_empty = true;
    for (uint8_t i = 0; i < sizeof(device_address); i++) {
        if (last_client_address.bytes[i] != 0) {
            is_last_client_address_empty = false;
            break;
        }
    }
    if (is_last_client_address_empty) {
#if CORE_DEBUG
        logger->log("no clients connected - shutting down gateway", 0);
#endif
        system->exit();
    }

    persistent->get_nth_client(0, client_id, &address, &status, &duration, &timeout);
    uint64_t i = 1;
    do {
        handle_client_publishes(status, client_id, &address);
        persistent->start_client_transaction(&address);
        if ((status == ACTIVE || status == AWAKE) && !persistent->has_client_publishes()) {
            persistent->set_client_state(DISCONNECTED);
            uint8_t transaction_return = persistent->apply_transaction();
            if (transaction_return == SUCCESS) {
                mqttsn->send_disconnect(&address);
            }
        } else {
            persistent->apply_transaction();
        }
        if (has_heart_beaten) {
            handle_timeout(status, duration, elapsed_time, client_id, address, timeout);
        }
        persistent->get_nth_client(i++, client_id, &address, &status, &duration, &timeout);
    } while (memcmp(&last_client_address, &address, sizeof(device_address)) != 0);
    handle_client_publishes(status, client_id, &address);
    persistent->start_client_transaction(&address);
    if ((status == ACTIVE || status == AWAKE) && !persistent->has_client_publishes()) {
        persistent->set_client_state(DISCONNECTED);
        uint8_t transaction_return = persistent->apply_transaction();
        if (transaction_return == SUCCESS) {
            mqttsn->send_disconnect(&address);
        }
    } else {
        persistent->apply_transaction();
    }
    if (has_heart_beaten) {
        handle_timeout(status, duration, elapsed_time, client_id, address, timeout);
    }

}

void CoreImpl::set_all_clients_lost() {
    device_address last_client_address;
    char client_id[24];
    device_address address;
    CLIENT_STATUS status;
    uint32_t timeout;
    uint32_t duration;

    persistent->get_last_client_address(&last_client_address);
    persistent->get_nth_client(0, client_id, &address, &status, &duration, &timeout);
    uint64_t i = 1;
    while (memcmp(&last_client_address, &address, sizeof(device_address)) != 0) {
        if (status != EMPTY) {
            persistent->start_client_transaction(&address);
            persistent->set_client_state(LOST);
            persistent->apply_transaction();
        }
        persistent->get_nth_client(i++, client_id, &address, &status, &duration, &timeout);
    }
}

void CoreImpl::handle_client_publishes(CLIENT_STATUS status, const char *client_id, device_address *address) {
    uint8_t transaction_return;
    if (status == AWAKE || status == ACTIVE) {
        persistent->start_client_transaction(address);
        if (persistent->get_client_await_message_type() == MQTTSN_PINGREQ && persistent->has_client_publishes()) {
            uint8_t databuffer[255];
            memset(&databuffer, 0, sizeof(databuffer));
            uint8_t data_len;
            uint16_t topic_id;
            bool retain;
            bool dup;
            uint8_t qos;
            uint16_t publish_id;
            persistent->get_next_publish(databuffer, &data_len, &topic_id, &retain, &qos, &dup, &publish_id);
            if (!persistent->is_topic_known(topic_id)) {
                // register first
                const char *topic_name = persistent->get_topic_name(topic_id);
                if (topic_name == nullptr) {
                    transaction_return = persistent->apply_transaction();
                    return;
                }

                uint16_t msg_id = persistent->get_client_await_msg_id();
                (msg_id + 1 == 0) ? msg_id = 1 : msg_id += 1;

                persistent->set_client_await_msg_id(msg_id);
                persistent->set_client_await_message(MQTTSN_REGACK);

                transaction_return = persistent->apply_transaction();
                if (transaction_return == SUCCESS) {
                    mqttsn->send_register(address, topic_id, msg_id, topic_name);
                }
                return;
            }

            if (qos == 0) {
                persistent->remove_publish_by_publish_id(publish_id);
                transaction_return = persistent->apply_transaction();
                if (transaction_return == SUCCESS) {
                    mqttsn->send_publish(address, databuffer, (uint8_t) data_len, 0, topic_id, true, retain,
                                         (uint8_t) qos,
                                         false);
                    return;
                }
                return;
            } else if (qos == 1) {
                uint16_t msg_id = persistent->get_client_await_msg_id();
                (msg_id + 1 == 0) ? msg_id = 1 : msg_id += 1;

                persistent->set_client_await_msg_id(msg_id);
                persistent->set_client_await_message(MQTTSN_PUBACK);

                persistent->set_publish_msg_id(publish_id, msg_id);

                transaction_return = persistent->apply_transaction();
                if (transaction_return == SUCCESS) {
                    mqttsn->send_publish(address, (uint8_t *) &databuffer, (uint8_t) data_len, msg_id, topic_id,
                                         true,
                                         retain,
                                         (uint8_t) qos, false);
                    return;
                }
                return;
            } else if (qos == 2) {
                // NOT_SUPPORTED qos 2 is not supported!
            }
        }
        transaction_return = persistent->apply_transaction();
    }
}

void CoreImpl::append_device_address(device_address *pAddress) {
    logger->append_log(" from ");
    char uint8_buf[5];
    for (uint8_t i = 0; i + 1 < sizeof(device_address); i++) {
        sprintf(uint8_buf, "%d", pAddress->bytes[i]);
        logger->append_log(uint8_buf);
        logger->append_log(".");
    }
    sprintf(uint8_buf, "%d", pAddress->bytes[sizeof(device_address) - 1]);
    logger->append_log(uint8_buf);
}
// THERE IS ALWAYS ONLY ONE MESSAGE IN FLIGHT

