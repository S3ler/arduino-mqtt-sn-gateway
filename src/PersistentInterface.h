#ifndef GATEWAY_PERSISTENT_H
#define GATEWAY_PERSISTENT_H


#include "core_defines.h"
#include "global_defines.h"
#include "mqttsn_messages.h"
#include "CoreInterface.h"
#include "LoggerInterface.h"
#include <stdint.h>

class Core;

// "in any other cases" means if an error occurs during a transaction or sth. like that
// if a pointer is given to this class, it has to be filled with information
// if possible use function with void return value, and you dont have to check for errors.
// Because in most cases we cannot handle error in the callee
// the design idea is: use it optimistically like no error internally happened. At the end check if an error occured.
// We use a transaction base read write, so we can improve the read/writer internally
// if we return a value, 0, everything 0 or nullptr are used for error cases
// CRUD for saved stuff
class PersistentInterface {

public: // initialization

    virtual ~PersistentInterface() {};

    virtual bool begin() = 0;

    virtual void setCore(Core *core)=0;

    virtual void setLogger(LoggerInterface *logger) = 0;

public: // transaction

    virtual void start_client_transaction(const char *client_id) = 0;

    virtual void start_client_transaction(device_address *address) = 0;

    virtual uint8_t apply_transaction() = 0;

public:

    /**
     * @return if the client exists in any state in the database
     */
    virtual bool client_exist() = 0;

    /**
     * removes a client, this action can only be performed when the client has no subscriptions.
     * Delete subscriptions first.
     * @param client_id
     */
    virtual void delete_client(const char *client_id)= 0;

    /**
     * adds a new client to the database
     * @param client_id
     * @param address
     */
    virtual void add_client(const char *client_id, device_address *address, uint32_t duration) = 0;

    /**
 * Resets a client from any state (e.g. disconnect or lost) to active, awating all messages and sets the client's addres to the given device_address
 * But does not change any other values like registrations, publishes or subscriptions
 * @param client_id
 * @param address
 */
    virtual void reset_client(const char *client_id, device_address *address, uint32_t duration)= 0;

    virtual void set_client_await_message(message_type msg_type)= 0;

    virtual message_type get_client_await_message_type() = 0;

    virtual void set_timeout(uint32_t timeout) = 0;

    virtual bool has_client_will()=0;

    virtual void get_client_will(char *target_willtopic, uint8_t *target_willmsg, uint8_t *target_willmsg_length,
                                 uint8_t *target_qos, bool *target_retain)=0;

    virtual void set_client_willtopic(char *willtopic, uint8_t qos, bool retain)=0;

    virtual void set_client_willmessage(uint8_t *willmsg, uint8_t willmsg_length)=0;

    virtual void delete_will() = 0;

    virtual void get_last_client_address(device_address *address) = 0;

    virtual void
    get_nth_client(uint64_t n, char *client_id, device_address *target_address, CLIENT_STATUS *target_status,
                   uint32_t *target_duration,
                   uint32_t *target_timeout) = 0;

public: // topic
    /**
     * Gets the topic name for a client by topic id
     * @param topic_id
     * @return
     * A pointer to the topic name or if the topic name does not exist NULL
     */
    virtual const char *get_topic_name(uint16_t topic_id) = 0;

    /**
    * Get the topic_id to the topic_name
    * Can be used to iterate over all short topics
    * @param topic_name
    * @return the topic_id or 0 if the entry does not exist
    */
    virtual uint16_t get_topic_id(char *topic_name)  = 0;

    virtual bool is_topic_known(uint16_t topic_id)=0;

    virtual bool set_topic_known(uint16_t topic_id, bool known)=0;

    virtual void add_client_registration(char *topic_name, uint16_t *topic_id) = 0;

    /**
    * Gets the topic name for a predefined topic by topic id
    * @param topic_id
    * @return
    * A pointer to the topic name or if the topic name does not exist NULL
    */
    virtual char *get_predefined_topic_name(uint16_t topic_id)  = 0;

    virtual void set_client_state(CLIENT_STATUS status) = 0;

    virtual void set_client_duration(uint32_t duration) = 0;

    virtual CLIENT_STATUS get_client_status()= 0;

    // return awaited msg_id
    // in any other case return 0
    virtual uint16_t get_client_await_msg_id()  = 0;

    virtual void set_client_await_msg_id(uint16_t msg_id)  = 0;


public: // subscription

    /**
    * Checks if a client is already subscribed to the given topic name
    * @param topic_name to check
    * @return true if the client is subscribed to the given topic
    *         false otherwise and in any other cases (especially when topic_id is NULL).
    */
    virtual bool is_subscribed(const char *topic_name) =0;

    virtual void add_subscription(const char *topic_name, uint16_t topic_id, uint8_t qos)=0;

    virtual void delete_subscription(uint16_t topic_id) = 0;

    virtual int8_t get_subscription_qos(const char *topic_name) = 0;

    virtual uint16_t get_subscription_topic_id(const char *topic_name) = 0;

    /**
 *
 * @return true if the client has at least 1 publish false otherwise and in any other cases.
 */
    virtual bool has_client_publishes()=0;

    /**
 * Get the i-th subscription of a short topic.
 * Can be used to iterate over all short topics
 * @param client_id
 * @param n entry
 * @return the topic_id or 0 if the entry does not exist
 */
    virtual uint16_t get_nth_subscribed_topic_id(uint16_t n)  = 0;

    /**
     * Get the number of topics a client is subscribed to
     * @return
     */
    virtual uint16_t get_client_subscription_count() = 0;

    //TODO check documentation
    /**
     * decrements the number of clients subscribed to the given topic
     * implementation tipp: if the subscription count reaches 0, delete the subscription
     * @param topic_name
     * @return true if the subscription exist (= subscription count is not 0) before the method is called
     *         false if the topic name does not exist in the subscription or any error appeared
     */
    virtual bool decrement_global_subscription_count(const char *topic_name) = 0;

    //TODO check documentation
    /**
     * increments the number of clients subscribed to the given topic
     * implementation tipp: if the subscription does not exist create it
     * @param topic_name
     * @return true if the subscription is incremented successfully
     *         false if any other error appeared (e.g. topic_name is NULL or an error during a transaction appeared).
     */
    virtual bool increment_global_subscription_count(const char *topic_name) = 0;

    /**
     * Returns the number of client subscribed to the given topic.
     * @param topic_name
     * @return
     * the count of subscribed clients to a topic name.
     * if the topic name does not exist or topic name is NULL return 0
     */
    virtual uint32_t get_global_topic_subscription_count(const char *topic_name) = 0;

public: // publish

    void
    add_new_client_publish(uint8_t *data, uint8_t data_len, uint16_t topic_id, bool retain,
                           uint8_t qos) {
        this->add_client_publish(data, data_len, topic_id, retain, qos, false, 0);
    }

    virtual void add_client_publish(uint8_t *data, uint8_t data_len, uint16_t topic_id, bool retain,
                                    uint8_t qos, bool dup, uint16_t msg_id) = 0;

    /**
     * Write data in the give pointer and removes it from saved publishes for the client
     * @param data
     * @param data_len put in data len 0 if an error occured
     * @param topic_id
     * @param short_topic
     * @param retain
     * @param qos
     * @param dup
     * @param publish_id
     * @return
     */
    virtual void
    get_next_publish(uint8_t *data, uint8_t *data_len, uint16_t *topic_id, bool *retain,
                     uint8_t *qos,
                     bool *dup, uint16_t *publish_id) =0;

    virtual void set_publish_msg_id(uint16_t publish_id, uint16_t msg_id)=0;

    virtual void remove_publish_by_msg_id(uint16_t msg_id)=0;

    virtual void remove_publish_by_publish_id(uint16_t msg_id)=0;

public: // gateway configuration

    virtual uint16_t get_advertise_duration() = 0;

    virtual bool get_gateway_id(uint8_t *gateway_id) = 0;

    virtual bool get_mqtt_config(uint8_t *server_ip, uint16_t *server_port, char *client_id) =0;

    virtual bool get_mqtt_login_config(char *username, char *password) = 0;

    virtual bool get_mqtt_will(char *will_topic, char *will_msg, uint8_t *will_qos, bool *will_retain)  = 0;


public: // gateway connection status

    virtual uint8_t set_mqttsn_disconnected()  = 0;

    virtual uint8_t set_mqtt_disconnected()  = 0;

    virtual uint8_t set_mqtt_connected()  = 0;

    virtual uint8_t set_mqttsn_connected()  = 0;

    virtual bool is_mqttsn_online()  = 0;

    virtual bool is_mqtt_online() = 0;

    // logging use only
    virtual void get_client_id(char *client_id)  = 0;

};


#endif //GATEWAY_PERSISTENT_H
