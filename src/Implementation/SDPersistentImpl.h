#include "SD_table_entries.h"
#include "../LoggerInterface.h"
#include "../mqttsn_messages.h"
#include "SDLinuxFake.h"
#include "Arduino.h"
#include <string.h>
#include <stdint.h>


#ifndef GATEWAY_PERSISTENTIMPL_H
#define GATEWAY_PERSISTENTIMPL_H


#define PERSISTENT_DEBUG 1

#define MAXIMUM_CLIENT_ID_LENGTH 24

#define MAXIMUM_TOPIC_NAME_LENGTH 255

#define SUBSCRIBE_FILE_ENDING ".SUB"

#define REGISTRATION_FILE_ENDING ".REG"

#define WILL_FILE_ENDING ".WIL"

#define PUBLISH_FILE_ENDING ".PUB"

class SDPersistentImpl : public PersistentInterface {

private:

#if PERSISTENT_DEBUG
    LoggerInterface *logger;
#endif
    SDLinuxFake SD;
    FileLinuxFake _open_file;
    SerialMock Serial;

    Core *core;

    entry_client _entry_client;
    entry_registration _registration_entry;
    entry_registration _predefined_registration_entry;

    bool _not_in_client_registry;
    bool _transaction_started;

    bool _error;

    const char *client_registry = "CLIENTS";
    const char *mqtt_sub = "MQTT.SUB";
    const char *predefined_topic = "TOPICS.PRE";
    const char *mqtt_configuration = "MQTT.CON";

private:


    size_t readCharUntil(char terminator, char *buffer, size_t buffer_size) {
        if (buffer_size < 1) return 0;
        size_t index = 0;
        while (index < buffer_size) {
            int c = _open_file.read();
            if (c < 0 || c == terminator) break;
            *buffer++ = (char) c;
            index++;
        }
        return index;
    }

    size_t readLine(char *buffer, size_t buffer_size) {
        return readCharUntil('\n', (char *) buffer, buffer_size);
    }

public:

    virtual bool begin() {
        if (core == nullptr) {
#if PERSISTENT_DEBUG
            logger->log("Error starting SDPersistentImpl: core is null ", 1);
#endif
            return false;
        }
        if (logger == nullptr) {
#if PERSISTENT_DEBUG
            logger->log("Error starting SDPersistentImpl: logger is null ", 1);
#endif
            return false;
        }
        // SD.begin cannot be called in this function.
        // it only works when called in the main.cpp/main.ino file
        /*
         *if (!SD.begin(4)) {
         *  logger->log(("SD card Initialization failed!");
         *   return false;
         *}
         */
        // ALWAYS: cast const char* to char* because there is a loop-bug in the SD-Card library
        //    if (SD.exists((char *) client_registry)) {
        //    }

        _transaction_started = false;
        _not_in_client_registry = false;

        create_file(client_registry);
        create_file(mqtt_sub);
#if PERSISTENT_DEBUG
        logger->log("SDPersistent ready", 1);
#endif
        return true;
    }

    void setRootPath(char* rootPath){
        SD.setRootPath(rootPath);
    }

    virtual void setCore(Core *core) {
        this->core = core;
    }

#if PERSISTENT_DEBUG

    virtual void setLogger(LoggerInterface *logger) {
        this->logger = logger;
    }

#endif

    virtual void start_client_transaction(const char *client_id) {
        if (_transaction_started) {
            _error = true;
            return;
        }

        if (strlen(client_id) >= MAXIMUM_CLIENT_ID_LENGTH) {
            _error = true;
            return;
        }
        _transaction_started = true;
        _error = false;
        _not_in_client_registry = true;

        _open_file.close();
        _open_file = SD.open(client_registry, FILE_READ);


#if PERSISTENT_DEBUG
        logger->start_log("start transaction by client id ", 3);
        logger->append_log(client_id);

#endif
        // if the file does not exist, it will be created
        uint32_t line_number = 0;
        int readChars = 0;
        do {
            memset(&_entry_client, 0, sizeof(entry_client));
            uint16_t buffer_size = sizeof(entry_client);
            readChars = _open_file.read(&_entry_client, buffer_size);
            if (readChars == buffer_size &&
                (strlen(_entry_client.client_id) < MAXIMUM_CLIENT_ID_LENGTH) &&
                (strcmp(_entry_client.client_id, client_id) == 0)) {
#if PERSISTENT_DEBUG
                logger->append_log(" - found. address ");
                char octed[4];
                sprintf(octed, "%d", _entry_client.client_address.bytes[0]);
                logger->append_log(octed);
                logger->append_log(".");
                sprintf(octed, "%d", _entry_client.client_address.bytes[1]);
                logger->append_log(octed);
                logger->append_log(".");
                sprintf(octed, "%d", _entry_client.client_address.bytes[2]);
                logger->append_log(octed);
                logger->append_log(".");
                sprintf(octed, "%d", _entry_client.client_address.bytes[3]);
                logger->append_log(octed);
                logger->append_log(".");
                sprintf(octed, "%d", _entry_client.client_address.bytes[4]);
                logger->append_log(octed);
                logger->append_log(".");
                sprintf(octed, "%d", _entry_client.client_address.bytes[5]);
                logger->append_log(octed);
                logger->append_log(" file number ");
                logger->append_log(_entry_client.file_number);

#endif
                _open_file.close();
                _not_in_client_registry = false;
                return;
            }
            line_number++;
        } while (readChars > 0);
#if PERSISTENT_DEBUG
        logger->append_log(" - client does not exist");
#endif
        _not_in_client_registry = true;
    }

    virtual void start_client_transaction(device_address *address) {
        if (_transaction_started) {
            _error = true;
            return;
        }
        _transaction_started = true;
        _error = false;
        _not_in_client_registry = false;

        _open_file.close();
        _open_file = SD.open(client_registry, FILE_READ);


#if PERSISTENT_DEBUG
        logger->start_log("start transaction by address ", 3);
        char octed[4];
        sprintf(octed, "%d", _entry_client.client_address.bytes[0]);
        logger->append_log(octed);
        logger->append_log(".");
        sprintf(octed, "%d", _entry_client.client_address.bytes[1]);
        logger->append_log(octed);
        logger->append_log(".");
        sprintf(octed, "%d", _entry_client.client_address.bytes[2]);
        logger->append_log(octed);
        logger->append_log(".");
        sprintf(octed, "%d", _entry_client.client_address.bytes[3]);
        logger->append_log(octed);
        logger->append_log(".");
        sprintf(octed, "%d", _entry_client.client_address.bytes[4]);
        logger->append_log(octed);
        logger->append_log(".");
        sprintf(octed, "%d", _entry_client.client_address.bytes[5]);
        logger->append_log(octed);
#endif

        uint32_t line_number = 0;
        int readChars = 0;
        do {
            memset(&_entry_client, 0, sizeof(entry_client));
            uint16_t buffer_size = sizeof(entry_client);

            readChars = _open_file.read(&_entry_client, buffer_size);
            if (readChars == buffer_size &&
                (strlen(_entry_client.client_id) < MAXIMUM_CLIENT_ID_LENGTH) &&
                (memcmp(&_entry_client.client_address, address, sizeof(device_address)) == 0)) {
#if PERSISTENT_DEBUG
                logger->append_log(" - found. client id ");
                logger->append_log(_entry_client.client_id);
                logger->append_log(" file number ");
                logger->append_log(_entry_client.file_number);
#endif
                _not_in_client_registry = false;
                _open_file.close();
                return;
            }
            line_number++;
        } while (readChars > 0);
#if PERSISTENT_DEBUG
        logger->append_log(" - client does not exist");
#endif
        _open_file.close();
        _not_in_client_registry = true;
    }

    virtual uint8_t apply_transaction() {
        _open_file.close();
        bool error = _error;
        bool transaction_started = _transaction_started;
        bool not_in_client_registry = _not_in_client_registry;
        _error = false;
        _transaction_started = false;
        _not_in_client_registry = false;


        if (transaction_started) {

            if (error) {
#if PERSISTENT_DEBUG
                logger->log("apply transaction - error", 1);
#endif
                return 0;
            }
            if(not_in_client_registry){
#if PERSISTENT_DEBUG
                logger->log("apply transaction - not in client registry", 1);
#endif
                return -1;
            }
#if PERSISTENT_DEBUG
            logger->log("apply transaction - success ", 3);
#endif
            return 1;
        }
#if PERSISTENT_DEBUG
        logger->log("apply transaction - no transaction started", 1);
#endif
        return 0;
    }

    virtual bool client_exist() {
        if (!_transaction_started || _error) {
            return false;
        }
        return !_not_in_client_registry;
    }

    virtual void reset_client(const char *client_id, device_address *address, uint32_t duration) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }
        if (strlen(client_id) >= MAXIMUM_CLIENT_ID_LENGTH) {
            _error = true;
            return;
        }

        if (strcmp(_entry_client.client_id, client_id) != 0) {
            _error = true;
            return;
        }

        _open_file = SD.open(client_registry, FILE_READ);

        // persistent the entry
        uint32_t line_number = 0;
        int readChars = 0;
        do {
            memset(&_entry_client, 0, sizeof(entry_client));
            uint16_t buffer_size = sizeof(entry_client);

            readChars = _open_file.read(&_entry_client, buffer_size);

            if (readChars == buffer_size &&
                (strlen(_entry_client.client_id) < MAXIMUM_CLIENT_ID_LENGTH) &&
                (strcmp(_entry_client.client_id, client_id) == 0)) {
                // set value
                _entry_client.client_status = ACTIVE;
                memcpy(&_entry_client.client_address, address, sizeof(device_address));
                memset(_entry_client.client_id, 0,
                       sizeof(_entry_client.client_id));
                strcpy(_entry_client.client_id, client_id);
                _entry_client.duration = duration;
                _entry_client.timeout = 0;
                _entry_client.client_status = ACTIVE;
                _entry_client.await_message_id = 0;
                _entry_client.await_message = MQTTSN_PINGREQ;
                // save
                _open_file.close();
                _open_file = SD.open(client_registry, FILE_WRITE);
                _open_file.seek(line_number * sizeof(entry_client));
                _open_file.write((const char *) &_entry_client, sizeof(entry_client));
                _not_in_client_registry = false;
                return;
            }
            line_number++;
        } while (readChars > 0);
        _error = true;
        _not_in_client_registry = true;
    }

    virtual void delete_client(const char *client_id) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }
        if (strlen(client_id) >= MAXIMUM_CLIENT_ID_LENGTH) {
            _error = true;
            return;
        }

        // check first if the client has subscriptions
        uint16_t subscription_count = this->get_client_subscription_count();
        if (subscription_count > 0) {
            _error = true;
            return;
        }


        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(REGISTRATION_FILE_ENDING)];

        // registration file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(REGISTRATION_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], REGISTRATION_FILE_ENDING,
               strlen(REGISTRATION_FILE_ENDING));
        delete_file(filename_with_extension);

        // subscription file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], SUBSCRIBE_FILE_ENDING,
               strlen(SUBSCRIBE_FILE_ENDING));
        delete_file(filename_with_extension);

        // will topic and message file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(WILL_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], WILL_FILE_ENDING, strlen(WILL_FILE_ENDING));
        delete_file(filename_with_extension);

        // publish messages file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(PUBLISH_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], PUBLISH_FILE_ENDING,
               strlen(PUBLISH_FILE_ENDING));
        delete_file(filename_with_extension);

        _open_file.close();
        _open_file = SD.open(client_registry, FILE_READ);

        uint32_t line_number = 0;
        int readChars = 0;
        do {
            memset(&_entry_client, 0, sizeof(entry_client));
            uint16_t buffer_size = sizeof(entry_client);
            readChars = _open_file.read((char *) &_entry_client, buffer_size);
            if (readChars == buffer_size &&
                (strlen(_entry_client.client_id) < MAXIMUM_CLIENT_ID_LENGTH) &&
                (strcmp(_entry_client.client_id, client_id) == 0)) {
                // found, now delete
                memset(&_entry_client, 0, sizeof(entry_client));

                _open_file.close();
                _open_file = SD.open(client_registry, FILE_WRITE);
                _open_file.seek(line_number * sizeof(entry_client));
                _open_file.write((const char *) &_entry_client, sizeof(entry_client));
                _not_in_client_registry = true;
                return;
            }
            line_number++;
        } while (readChars > 0);
        _not_in_client_registry = true;
    }


    void create_file(const char *file_path) {
        _open_file.close();
        _open_file = SD.open(file_path, FILE_WRITE);
        _open_file.close();
    }


    void delete_file(const char *file_path) {
        _open_file.close();
        if (SD.exists((char *) file_path)) {
            SD.remove((char *) file_path);
        }
    }


    virtual void add_client(const char *client_id, device_address *address, uint32_t duration) {
        if (!_transaction_started) {
            return;
        }
        if (_error) {
            return;
        }
        if (!_not_in_client_registry) {
            _error = true;
            return;
        }

        if (strlen(client_id) >= MAXIMUM_CLIENT_ID_LENGTH) {
            _error = true;
            return;
        }

#if PERSISTENT_DEBUG
        logger->start_log("add client ", 3);
        logger->append_log(client_id);
        logger->append_log(" ");
        char octed[10];
        sprintf(octed, "%d", address->bytes[0]);
        logger->append_log(octed);
        logger->append_log(".");
        sprintf(octed, "%d", address->bytes[1]);
        logger->append_log(octed);
        logger->append_log(".");
        sprintf(octed, "%d", address->bytes[2]);
        logger->append_log(octed);
        logger->append_log(".");
        sprintf(octed, "%d", address->bytes[3]);
        logger->append_log(octed);
        logger->append_log(".");
        sprintf(octed, "%d", address->bytes[4]);
        logger->append_log(octed);
        logger->append_log(".");
        sprintf(octed, "%d", address->bytes[5]);
        logger->append_log(octed);
        logger->append_log(" duration ");
        sprintf(octed, "%d", duration);
        logger->append_log(octed);
#endif

        uint32_t empty_space = find_empty_entry_space_in_registry();

        memset(&_entry_client, 0x0, sizeof(entry_client));
        strcpy(_entry_client.client_id, client_id);
        sprintf(_entry_client.file_number, "%08d", empty_space);
        memcpy(&_entry_client.client_address, address, sizeof(device_address));
        _entry_client.duration = duration;
        _entry_client.timeout = 0;
        _entry_client.client_status = ACTIVE;
        _entry_client.await_message_id = 0;
        _entry_client.await_message = MQTTSN_PINGREQ;
        //memset(_entry_client.file_number, '0', sizeof(_entry_client.file_number));


        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(REGISTRATION_FILE_ENDING)];

        // registration file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(REGISTRATION_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], REGISTRATION_FILE_ENDING,
               strlen(REGISTRATION_FILE_ENDING));
        create_file(filename_with_extension);

        // subscription file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], SUBSCRIBE_FILE_ENDING,
               strlen(SUBSCRIBE_FILE_ENDING));
        create_file(filename_with_extension);

        // will topic and message file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(WILL_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], WILL_FILE_ENDING, strlen(WILL_FILE_ENDING));
        create_file(filename_with_extension);

        // publish messages file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(PUBLISH_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], PUBLISH_FILE_ENDING,
               strlen(PUBLISH_FILE_ENDING));
        create_file(filename_with_extension);

        _open_file = SD.open(client_registry, FILE_WRITE);
        _open_file.seek(empty_space * sizeof(entry_client));
        _open_file.write((const char *) &_entry_client, sizeof(entry_client));
        _open_file.close();
#if PERSISTENT_DEBUG
        logger->append_log(" - success file number ");
        logger->append_log(_entry_client.file_number);
#endif
        _not_in_client_registry = false;
        // TODO check
    }


    virtual uint16_t get_client_subscription_count(const char *client_id) {
        return 0;
    }

    virtual char *get_predefined_topic_name(uint16_t topic_id) {

        if (_error) {
            return nullptr;
        }
        _open_file.flush();
        _open_file.close();
        // CHECKME
        _open_file = SD.open(predefined_topic, FILE_READ);

        char buffer[262];
        int readChars = 0;
        memset(&_predefined_registration_entry, 0, sizeof(entry_registration));
        do {
            memset(&buffer, 0, sizeof(buffer));
            readChars = readLine(buffer, sizeof(buffer));
            if (readChars > 3) {
                char *found = strtok(buffer, " ");
                if (found == NULL) {
                    continue;
                }
                uint8_t strtok_entries = 0;
                while (found) {
                    uint16_t found_topic_id = (uint16_t) atoi(found);
                    if (found_topic_id > UINT16_MAX) {
                        break;
                    }
                    strtok_entries++;
                    found = strtok(NULL, " ");
                    char *found_topic_name = found;
                    if (strlen(found_topic_name) >= MAXIMUM_TOPIC_NAME_LENGTH) {
                        break;
                    }

                    strtok_entries++;
                    // now we have parsed a predefined topic
                    if (found_topic_id == topic_id) {
                        // found it
                        _predefined_registration_entry.topic_id = found_topic_id;
                        strcpy((char *) &_predefined_registration_entry.topic_name, found_topic_name);
                        return _predefined_registration_entry.topic_name;
                    }
                    if (strtok_entries >= 2) {
                        break;
                    }
                }
            }
        } while (readChars > 0);
        return nullptr;
    }




    virtual void delete_will() {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }

        _open_file.close();
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(REGISTRATION_FILE_ENDING)];
        // will topic and message file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(WILL_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], WILL_FILE_ENDING, strlen(WILL_FILE_ENDING));

        SD.remove(filename_with_extension);
        create_file(filename_with_extension);
    }




    bool parse_uint8_t_after_space(uint8_t *destination, const char *source, uint16_t max) {
        const char *cpy_source = source;
        char tmp_number[4];
        memset(tmp_number, 0, sizeof(tmp_number));
        // see: https://github.com/esp8266/Arduino/issues/572
        // char space[] = " ";
        // uint16_t start = (uint16_t) strcspn(source, space);
        uint16_t start = 0;
        while (*source++ != ' ') {
            start++;
            if (*source == '\n' || start == max) {
                return false;
            }
        }
        // see: https://github.com/esp8266/Arduino/issues/572
        // char newline[] = "";
        // uint16_t end = (uint16_t) strcspn(source, newline);
        uint16_t end = start;
        while (*source != '\n' && *source != 0) {
            *source++;
            end++;
            if (end > max) {
                return false;
            }
        }

        if (start + 1 < max && start != end && (end - start) <= 3 && (end - start) >= 1) {
            memcpy(tmp_number, &cpy_source[start + 1], end - start);
            *destination = (uint8_t) atoi(tmp_number);
            return true;
        }
        return false;
    }

    bool parse_uint16_t_after_space(uint16_t *destination, const char *source, uint16_t max) {
        // lesson learned:
        // readBytesUntil does not include the untilByte
        // this means we also check for 0 (additionally to \n)
        // save a copy of the source pointer for the memcpy

        const char *cpy_source = source;
        char tmp_number[7];
        memset(tmp_number, 0, sizeof(tmp_number));
        // see: https://github.com/esp8266/Arduino/issues/572
        // char space[] = " ";
        // uint16_t start = (uint16_t) strcspn(source, space);
        uint16_t start = 0;
        while (*source++ != ' ') {
            start++;
            if (*source == '\n' || start == max) {
                return false;
            }
        }
        // see: https://github.com/esp8266/Arduino/issues/572
        // char newline[] = "";
        // uint16_t end = (uint16_t) strcspn(source, newline);
        uint16_t end = start;
        while (*source != '\n' && *source != 0) {
            *source++;
            end++;
            if (end > max) {
                return false;
            }
        }

        if (start + 1 < max && start != end && (end - start) <= 6 && (end - start) >= 1) {
            memcpy(tmp_number, &cpy_source[start + 1], end - start);
            *destination = (uint16_t) atoi(tmp_number);
            if (*destination == 0) {
                return false;
            }
            return true;
        }
        return false;
    }

    bool parse_string_after_space(char *destination, const char *source, uint16_t max) const {

        const char *cpy_source = source;
        // see: https://github.com/esp8266/Arduino/issues/572
        // char space[] = " ";
        // uint16_t start = (uint16_t) strcspn(source, space);
        uint16_t start = 0;
        while (*source++ != ' ') {
            start++;
            if (*source == '\n' || start == max) {
                return false;
            }
        }
        // see: https://github.com/esp8266/Arduino/issues/572
        // char newline[] = "";
        // uint16_t end = (uint16_t) strcspn(source, newline);
        uint16_t end = start;
        while (*source != '\n' && *source != 0) {
            *source++;
            end++;
            if (end > max) {
                return false;
            }
        }
        if (start + 1 < max && start != end) {
            memcpy(destination, &cpy_source[start + 1], end - start);
            if (strlen(destination) == 0) {
                return false;
            }
            return true;
        }
        return false;
    }


    bool parse_ipaddress_after_space(uint8_t *destination, const char *source, uint16_t max) {
        const char *cpy_source = source;
        char tmp_only_id[17];
        memset(tmp_only_id, 0, sizeof(tmp_only_id));

        // see: https://github.com/esp8266/Arduino/issues/572
        // char space[] = " ";
        // uint16_t start = (uint16_t) strcspn(source, space);
        uint16_t start = 0;
        while (*source++ != ' ') {
            start++;
            if (*source == '\n' || start == max) {
                return false;
            }
        }
        // see: https://github.com/esp8266/Arduino/issues/572
        // char newline[] = "";
        // uint16_t end = (uint16_t) strcspn(source, newline);
        uint16_t end = start;
        while (*source != '\n' && *source != 0) {
            *source++;
            end++;
            if (end > max) {
                return false;
            }
        }

        if (start + 1 < max && start != end && (end - start) < 17 && (end - start) > 7) {
            memcpy(tmp_only_id, &cpy_source[start + 1], end - start);
            char *found = strtok(tmp_only_id, ".");
            if (found == NULL) {
                return false;
            }
            uint8_t numbers = 0;
            while (found) {
                uint16_t tmp_number = (uint16_t) atoi(found);
                if (tmp_number > 255) {
                    return false;
                }
                *(destination + numbers) = (uint8_t) tmp_number;
                numbers++;
                found = strtok(NULL, ".");
                if (found == NULL && numbers == 4) {
                    return true;
                }
                if (found != NULL && numbers == 4) {
                    return false;
                }
            }
            return false;
        }
        return false;
    }


    virtual uint16_t get_topic_id(char *topic_name) {
        return 0;
    }


    virtual bool increment_global_subscription_count(const char *topic_name) {
        if (_error) {
            return false;
        }
        if (topic_name == nullptr || strlen(topic_name) == 0 || strlen(topic_name) >= MAXIMUM_TOPIC_NAME_LENGTH) {
            return false;
        }

        if (strlen(topic_name) > MAXIMUM_TOPIC_NAME_LENGTH) {
            _error = true;
            return false;
        }
        _open_file.close();

        _open_file = SD.open(mqtt_sub, FILE_READ);

#if PERSISTENT_DEBUG
        logger->start_log("increment_topic_subscription ", 3);
        logger->append_log(topic_name);
#endif

        entry_mqtt_subscription _entry_mqtt_subscription;
        uint16_t entry_number = 0;
        int readChars = 0;
        int32_t first_empty_space = -1;
        do {
            memset(&_entry_mqtt_subscription, 0, sizeof(entry_mqtt_subscription));
            uint16_t buffer_size = sizeof(entry_mqtt_subscription);
            readChars = _open_file.read((char *) &_entry_mqtt_subscription, buffer_size);
            if (readChars == buffer_size) {
                if (_entry_mqtt_subscription.client_subscription_count == 0 &&
                    strlen(_entry_mqtt_subscription.topic_name) == 0) {
                    // empty place found
                    first_empty_space = entry_number;
                } else if (_entry_mqtt_subscription.client_subscription_count != 0 &&
                           strlen(_entry_mqtt_subscription.topic_name) < MAXIMUM_TOPIC_NAME_LENGTH &&
                           (strcmp(_entry_mqtt_subscription.topic_name, topic_name) == 0)) {
                    // found entry

                    // increment
                    _entry_mqtt_subscription.client_subscription_count += 1;

#if PERSISTENT_DEBUG
                    logger->append_log(" exists - count ");
                    char uint16_buf[6];
                    sprintf(uint16_buf, "%d", _entry_mqtt_subscription.client_subscription_count);
                    logger->append_log(uint16_buf);
#endif
                    // save back
                    _open_file = SD.open(mqtt_sub, FILE_WRITE);
                    _open_file.seek(entry_number * sizeof(entry_mqtt_subscription));
                    _open_file.write((const char *) &_entry_mqtt_subscription, buffer_size);

                    return true;
                }
            } else if (readChars != 0 && readChars < buffer_size) {
                break;
            }
            entry_number++;
        } while (readChars > 0);
        if (first_empty_space == -1) {
            // append registration
            first_empty_space = entry_number - 1;
        }

        strcpy((char *) &_entry_mqtt_subscription.topic_name, topic_name);
        _entry_mqtt_subscription.client_subscription_count = 1;

#if PERSISTENT_DEBUG
        logger->append_log(" not exist - count ");
        char uint16_buf[6];
        sprintf(uint16_buf, "%d", _entry_mqtt_subscription.client_subscription_count);
        logger->append_log(uint16_buf);
#endif

        _open_file.close();
        _open_file = SD.open(mqtt_sub, FILE_WRITE);

        //_open_file.seekg(0, _open_file.beg);
        _open_file.seek(first_empty_space * sizeof(entry_mqtt_subscription));
        uint16_t buffer_size = sizeof(entry_mqtt_subscription);
        _open_file.write((const char *) &_entry_mqtt_subscription, buffer_size);
        _open_file.close();

        // check if subscription exist
        // if yes increment
        // if no, find empty space and increment
        return true;
    }

    virtual bool is_subscribed(const char *topic_name) {
        if (!_transaction_started || _error) {
            return false;
        }
        if (_not_in_client_registry) {
            return false;
        }

        if (topic_name == nullptr || strlen(topic_name) == 0 || strlen(topic_name) >= MAXIMUM_TOPIC_NAME_LENGTH) {
            return false;
        }
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING)];

        // subscription file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], SUBSCRIBE_FILE_ENDING,
               strlen(SUBSCRIBE_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);

#if PERSISTENT_DEBUG
        char uint16_buf[6];
        logger->start_log("has subscription ", 3);
        logger->append_log(topic_name);
#endif

        entry_subscription _entry_subscription;
        int readChars = 0;
        do {
            memset(&_entry_subscription, 0, sizeof(entry_subscription));
            uint16_t buffer_size = sizeof(entry_subscription);
            readChars = _open_file.read((char *) &_entry_subscription, buffer_size);
            if (readChars == buffer_size) {
                if (strlen(_entry_subscription.topic_name) < MAXIMUM_TOPIC_NAME_LENGTH &&
                    (strcmp(_entry_subscription.topic_name, topic_name) == 0)) {
                    // already subscribed
#if PERSISTENT_DEBUG
                    logger->append_log(" - already subscribed");
                    return true;
#endif
                }
            }
        } while (readChars > 0);
#if PERSISTENT_DEBUG
        logger->append_log(" - not subscribed");
#endif
        return false;
    }

    virtual void add_subscription(const char *topic_name, uint16_t topic_id, uint8_t qos) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }
        if (topic_name == nullptr || strlen(topic_name) == 0 || strlen(topic_name) >= MAXIMUM_TOPIC_NAME_LENGTH) {
            return;
        }
        if (topic_id == 0) {
            return;
        }
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING)];

        // subscription file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], SUBSCRIBE_FILE_ENDING,
               strlen(SUBSCRIBE_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);

#if PERSISTENT_DEBUG
        char uint16_buf[6];
        logger->start_log("subscribe topic ", 3);
        logger->append_log(topic_name);

        logger->append_log(" topic id ");
        sprintf(uint16_buf, "%d", topic_id);
        logger->append_log(uint16_buf);

        logger->append_log(" qos ");
        sprintf(uint16_buf, "%d", qos);
        logger->append_log(uint16_buf);
#endif

        entry_subscription _entry_subscription;

        uint16_t entry_number = 0;
        int readChars = 0;
        int32_t first_empty_space = -1;
        do {
            memset(&_entry_subscription, 0, sizeof(entry_subscription));
            uint16_t buffer_size = sizeof(entry_subscription);
            readChars = _open_file.read((char *) &_entry_subscription, buffer_size);
            if (readChars == buffer_size) {
                if (_entry_subscription.topic_id == 0 &&
                    strlen(_entry_subscription.topic_name) == 0) {
                    // empty place found
                    first_empty_space = entry_number;
                } else if (_entry_subscription.topic_id == topic_id &&
                           strlen(_entry_subscription.topic_name) < MAXIMUM_TOPIC_NAME_LENGTH &&
                           (strcmp(_entry_subscription.topic_name, topic_name) == 0)) {
                    // already subscribed
#if PERSISTENT_DEBUG
                    logger->append_log(" - already subscribed");
                    return;
#endif
                }
            }
            entry_number++;
        } while (readChars > 0);
        if (first_empty_space == -1) {
            // append subscription
            first_empty_space = entry_number - 1;
        }
        memset(&_entry_subscription, 0, sizeof(_entry_subscription));
        _entry_subscription.topic_id = topic_id;
        _entry_subscription.qos = qos;
        strcpy(_entry_subscription.topic_name, topic_name);

        _open_file.close();
        _open_file = SD.open(filename_with_extension, FILE_WRITE);
        _open_file.seek(first_empty_space * sizeof(entry_subscription));
        uint16_t buffer_size = sizeof(entry_subscription);
        _open_file.write((const char *) &_entry_subscription, buffer_size);
        _open_file.close();
#if PERSISTENT_DEBUG
        logger->append_log(" - saved at position ");
        sprintf(uint16_buf, "%d", first_empty_space);
        logger->append_log(uint16_buf);
#endif
    }

private:

    uint32_t find_empty_entry_space_in_registry() {
        _open_file.flush();
        _open_file.close();
        _open_file = SD.open(client_registry, FILE_READ);


        size_t readChars = 0;
        uint16_t entry_number = 0;
        do {
            memset(&_entry_client, 0, sizeof(entry_client));
            size_t buffer_size = sizeof(entry_client);
            readChars = _open_file.read((char *) &_entry_client, buffer_size);
            if (readChars == 0) {
                // end of file reached
                break;
            }

            if (strlen(_entry_client.client_id) == 0) {
                // empty space found
                return entry_number;
            }
            entry_number++;
        } while (readChars > 0);
        return entry_number;
    }

    virtual uint16_t get_client_subscription_count() {
        if (!_transaction_started || _error) {
#if  PERSISTENT_DEBUG
            Serial.println("get_client_subscription_count error");
#endif
            return 0;
        }
        if (_not_in_client_registry) {
#if  PERSISTENT_DEBUG
            Serial.println("get_client_subscription_count _not_in_client_registry");
#endif
            return 0;
        }
        _open_file.flush();
        _open_file.close();

        // subscription file
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING)];
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], SUBSCRIBE_FILE_ENDING,
               strlen(SUBSCRIBE_FILE_ENDING));

        _open_file = SD.open(filename_with_extension, FILE_READ);

        uint16_t entry_number = 0;
        int readChars = 0;
        entry_subscription entry;
        do {
            memset(&entry, 0, sizeof(entry_subscription));
            uint16_t buffer_size = sizeof(entry_subscription);
            readChars = _open_file.read((char *) &entry, buffer_size);

            if (readChars == buffer_size &&
                entry.topic_id != 0) {
                entry_number++;
            }
        } while (readChars > 0);
        return entry_number;
    }

    virtual uint16_t get_nth_subscribed_topic_id(uint16_t n) {
        if (!_transaction_started || _error) {
#if  PERSISTENT_DEBUG
            Serial.println("get_client_topic_id error");
#endif
            return 0;
        }
        _open_file.flush();
        _open_file.close();

        // subscription file
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING)];
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], SUBSCRIBE_FILE_ENDING,
               strlen(SUBSCRIBE_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);

        _open_file.seek(n * sizeof(entry_subscription));
        entry_subscription entry;
        memset(&entry, 0, sizeof(entry_subscription));
        uint16_t buffer_size = sizeof(entry_subscription);
        int readChars = _open_file.read((char *) &entry, buffer_size);

        if (readChars == buffer_size &&
            entry.topic_id != 0) {
            return entry.topic_id;
        }
        return 0;
    }

    virtual
    const char * get_topic_name(uint16_t topic_id) {
        if (!_transaction_started || _error) {
#if PERSISTENT_DEBUG
            Serial.println("get_client_topic_name error");
#endif
            return nullptr;
        }
        if (topic_id == 0) {
            return nullptr;
        }
        _open_file.flush();
        _open_file.close();

        // registration file
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(REGISTRATION_FILE_ENDING)];
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(REGISTRATION_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], REGISTRATION_FILE_ENDING,
               strlen(REGISTRATION_FILE_ENDING));

        _open_file = SD.open(filename_with_extension, FILE_READ);

        uint16_t entry_number = 0;
        int readChars = 0;
        do {
            memset(&_registration_entry, 0, sizeof(entry_registration));
            uint16_t buffer_size = sizeof(entry_registration);
            readChars = _open_file.read((char *) &_registration_entry, buffer_size);

            if (readChars == buffer_size &&
                _registration_entry.topic_id == topic_id &&
                strlen(_registration_entry.topic_name) < MAXIMUM_TOPIC_NAME_LENGTH) {
                return _registration_entry.topic_name;
            }
        } while (readChars > 0);
        return nullptr;
    }

    virtual void delete_subscription(uint16_t topic_id) {
        if (!_transaction_started || _error) {
            Serial.println("delete_client_subscription _error");
            return;
        }
        if (topic_id == 0) {
            Serial.println("delete_client_subscription topic_id");
            return;
        }
        _open_file.flush();
        _open_file.close();

        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING)];

        // subscription file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], SUBSCRIBE_FILE_ENDING,
               strlen(SUBSCRIBE_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);

        int readChars = 0;
        uint16_t line_number = 0;
        entry_subscription entry;
        do {
            memset(&entry, 0, sizeof(entry_subscription));
            uint16_t buffer_size = sizeof(entry_subscription);
            readChars = _open_file.read((char *) &entry, buffer_size);

            if (readChars == buffer_size &&
                entry.topic_id == topic_id) {
                // found, now delete
                _open_file.close();
                _open_file = SD.open(filename_with_extension, FILE_WRITE);
                memset(&entry, 0, sizeof(entry_subscription));
                if (_open_file.seek(line_number * sizeof(entry_subscription))) {
                    _open_file.write((const char *) &entry, sizeof(entry_subscription));
                    Serial.println(" subscription deleted");
                    return;
                }
            }
            line_number++;
        } while (readChars > 0);
        Serial.println(" in function delete_client_subscription_error end");
        _error = true;
    }

    virtual bool decrement_global_subscription_count(const char *topic_name) {
        if (_error) {
            return false;
        }
        if (topic_name == nullptr || strlen(topic_name) == 0 || strlen(topic_name) >= MAXIMUM_TOPIC_NAME_LENGTH) {
            return false;
        }

        if (strlen(topic_name) > MAXIMUM_TOPIC_NAME_LENGTH) {
            _error = true;
            return false;
        }
        _open_file.close();

        _open_file = SD.open(mqtt_sub, FILE_READ);

#if PERSISTENT_DEBUG
        logger->start_log("decrement_topic_subscription ", 3);
        logger->append_log(topic_name);
#endif

        entry_mqtt_subscription _entry_mqtt_subscription;
        uint16_t entry_number = 0;
        int readChars = 0;
        int32_t first_empty_space = -1;
        do {
            memset(&_entry_mqtt_subscription, 0, sizeof(entry_mqtt_subscription));
            uint16_t buffer_size = sizeof(entry_mqtt_subscription);
            readChars = _open_file.read((char *) &_entry_mqtt_subscription, buffer_size);
            if (readChars == buffer_size) {
                if (_entry_mqtt_subscription.client_subscription_count == 0 &&
                    strlen(_entry_mqtt_subscription.topic_name) == 0) {
                    // empty place found
                    first_empty_space = entry_number;
                } else if (_entry_mqtt_subscription.client_subscription_count != 0 &&
                           strlen(_entry_mqtt_subscription.topic_name) < MAXIMUM_TOPIC_NAME_LENGTH &&
                           (strcmp(_entry_mqtt_subscription.topic_name, topic_name) == 0)) {
                    // found entry

                    // decrement
                    _entry_mqtt_subscription.client_subscription_count -= 1;

#if PERSISTENT_DEBUG
                    logger->append_log(" exists - count ");
                    char uint16_buf[6];
                    sprintf(uint16_buf, "%d", _entry_mqtt_subscription.client_subscription_count);
                    logger->append_log(uint16_buf);
#endif
                    if (_entry_mqtt_subscription.client_subscription_count == 0) {
                        memset(&_entry_mqtt_subscription, 0, sizeof(entry_mqtt_subscription));
                    }

                    // save back
                    _open_file = SD.open(mqtt_sub, FILE_WRITE);
                    _open_file.seek(entry_number * sizeof(entry_mqtt_subscription));
                    _open_file.write((const char *) &_entry_mqtt_subscription, buffer_size);

                    return true;
                }
            } else if (readChars != 0 && readChars < buffer_size) {
                break;
            }
            entry_number++;
        } while (readChars > 0);
#if PERSISTENT_DEBUG
        logger->append_log(" not exist");
#endif
        return true;
    }



    virtual void add_client_registration(char *topic_name, uint16_t *new_topic_id) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }
        if (strlen(topic_name) > MAXIMUM_TOPIC_NAME_LENGTH) {
            _error = true;
            return;
        }
        _open_file.flush();
        _open_file.close();

        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(REGISTRATION_FILE_ENDING)];

        // registration file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(REGISTRATION_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], REGISTRATION_FILE_ENDING,
               strlen(REGISTRATION_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);
#if PERSISTENT_DEBUG
        logger->start_log("register topic ", 3);
        logger->append_log(topic_name);
#endif

        uint16_t entry_number = 0;
        int readChars = 0;
        int32_t first_empty_space = -1;
        do {
            memset(&_registration_entry, 0, sizeof(entry_registration));
            uint16_t buffer_size = sizeof(entry_registration);
            readChars = _open_file.read((char *) &_registration_entry, buffer_size);
            if (readChars == buffer_size) {
                if (_registration_entry.topic_id == 0 &&
                    strlen(_registration_entry.topic_name) == 0) {
                    // empty place found
                    first_empty_space = entry_number;
                } else if (_registration_entry.topic_id != 0 &&
                           strlen(_registration_entry.topic_name) < MAXIMUM_TOPIC_NAME_LENGTH &&
                           (strcmp(_registration_entry.topic_name, topic_name) == 0)) {
                    // already_registered
#if PERSISTENT_DEBUG
                    logger->append_log(" - already registered topic id ");
                    char uint16_buf[6];
                    sprintf(uint16_buf, "%d", _registration_entry.topic_id);
                    logger->append_log(uint16_buf);
#endif

                    *new_topic_id = _registration_entry.topic_id;
                    _registration_entry.known = true;
                    return;
                }
            } else if (readChars != 0 && readChars < buffer_size) {
                break;
            }
            entry_number++;
        } while (readChars > 0);
        if (first_empty_space == -1) {
            // append registration
            first_empty_space = entry_number - 1;
        }
        // save at first_empty_space position
        _registration_entry.topic_id = entry_number;
        strcpy((char *) &_registration_entry.topic_name, topic_name);
        _registration_entry.known = true;

        _open_file.close();
        _open_file = SD.open(filename_with_extension, FILE_WRITE);
        //_open_file.seekg(0, _open_file.beg);
        _open_file.seek(first_empty_space * sizeof(entry_registration));
        uint16_t buffer_size = sizeof(entry_registration);
        _open_file.write((const char *) &_registration_entry, buffer_size);
        _open_file.close();
        *new_topic_id = _registration_entry.topic_id;
#if PERSISTENT_DEBUG
        logger->append_log("- saved topic id ");
        char uint16_buf[6];
        sprintf(uint16_buf, "%d", _registration_entry.topic_id);
        logger->append_log(uint16_buf);
#endif
    }


public:





    virtual bool is_topic_known(uint16_t topic_id) {
        if (!_transaction_started || _error) {
            return false;
        }
        if (_not_in_client_registry) {
            return false;
        }
        if (topic_id == 0) {
            _error = true;
            return false;
        }
        _open_file.flush();
        _open_file.close();

        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(REGISTRATION_FILE_ENDING)];

        // registration file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(REGISTRATION_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], REGISTRATION_FILE_ENDING,
               strlen(REGISTRATION_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);
#if PERSISTENT_DEBUG
        logger->start_log("is_topic_known_by_client ", 3);
        char uint16_buf[6];
        sprintf(uint16_buf, "%d", _registration_entry.topic_id);
        logger->append_log(uint16_buf);

#endif

        uint16_t entry_number = 0;
        int readChars = 0;
        int32_t first_empty_space = -1;
        do {
            memset(&_registration_entry, 0, sizeof(entry_registration));
            uint16_t buffer_size = sizeof(entry_registration);
            readChars = _open_file.read((char *) &_registration_entry, buffer_size);
            if (readChars == buffer_size) {
                if (_registration_entry.topic_id == 0 &&
                    strlen(_registration_entry.topic_name) == 0) {
                    // empty place found
                    first_empty_space = entry_number;
                } else if (_registration_entry.topic_id == topic_id &&
                           strlen(_registration_entry.topic_name) < MAXIMUM_TOPIC_NAME_LENGTH) {
                    // already_registered
#if PERSISTENT_DEBUG
                    if (_registration_entry.known) {
                        logger->append_log(" - known ");
                    } else {
                        logger->append_log(" - unkown ");
                    }
                    logger->append_log(_registration_entry.topic_name);
#endif
                    return _registration_entry.known;
                }
            } else if (readChars != 0 && readChars < buffer_size) {
                break;
            }
            entry_number++;
        } while (readChars > 0);
    }

    virtual bool set_topic_known(uint16_t topic_id, bool known){

    }



    virtual void set_client_await_message(message_type msg_type) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }
#if PERSISTENT_DEBUG
        logger->start_log("await message ", 3);
        char buf[10];
        sprintf(buf, "%d", msg_type);
        logger->append_log(buf);
#endif

        int client_position = parse_file_number_to_int(&_entry_client);
        _entry_client.await_message = msg_type;
        _open_file = SD.open(client_registry, FILE_WRITE);
        _open_file.seek(client_position * sizeof(entry_client));
        _open_file.write((const char *) &_entry_client, sizeof(entry_client));
        _open_file.close();

    }


    virtual void set_client_duration(uint32_t duration) {

    }



    virtual CLIENT_STATUS get_client_status() {
        if (!_transaction_started || _error) {
            return LOST;
        }
        if (_not_in_client_registry) {
            return LOST;
        }
        return _entry_client.client_status;
    }


    virtual int8_t get_subscription_qos(const char *topic_name) {
        if (!_transaction_started || _error) {
            return false;
        }
        if (_not_in_client_registry) {
            return false;
        }

        if (topic_name == nullptr || strlen(topic_name) == 0 || strlen(topic_name) >= MAXIMUM_TOPIC_NAME_LENGTH) {
            return false;
        }
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING)];

        // subscription file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], SUBSCRIBE_FILE_ENDING,
               strlen(SUBSCRIBE_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);

#if PERSISTENT_DEBUG
        logger->start_log("subscription qos of ", 3);
        logger->append_log(topic_name);
#endif

        entry_subscription _entry_subscription;
        int readChars = 0;
        do {
            memset(&_entry_subscription, 0, sizeof(entry_subscription));
            uint16_t buffer_size = sizeof(entry_subscription);
            readChars = _open_file.read((char *) &_entry_subscription, buffer_size);
            if (readChars == buffer_size) {
                if (strlen(_entry_subscription.topic_name) < MAXIMUM_TOPIC_NAME_LENGTH &&
                    (strcmp(_entry_subscription.topic_name, topic_name) == 0)) {
                    // already subscribed
#if PERSISTENT_DEBUG
                    char uint16_buf[6];
                    sprintf(uint16_buf, "%d", _entry_subscription.qos);
                    logger->append_log(" - ");
                    logger->append_log(uint16_buf);
#endif
                    return _entry_subscription.qos;
                }
            }
        } while (readChars > 0);
#if PERSISTENT_DEBUG
        logger->append_log(" - not subscribed");
#endif
        return -1;
    }

    virtual uint16_t get_subscription_topic_id(const char *topic_name) {
        if (!_transaction_started || _error) {
            return false;
        }
        if (_not_in_client_registry) {
            return false;
        }

        if (topic_name == nullptr || strlen(topic_name) == 0 || strlen(topic_name) >= MAXIMUM_TOPIC_NAME_LENGTH) {
            return false;
        }
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING)];

        // subscription file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(SUBSCRIBE_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], SUBSCRIBE_FILE_ENDING,
               strlen(SUBSCRIBE_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);

#if PERSISTENT_DEBUG
        char uint16_buf[6];
        logger->start_log("subscription topic id of ", 3);
        logger->append_log(topic_name);
#endif

        entry_subscription _entry_subscription;
        int readChars = 0;
        do {
            memset(&_entry_subscription, 0, sizeof(entry_subscription));
            uint16_t buffer_size = sizeof(entry_subscription);
            readChars = _open_file.read((char *) &_entry_subscription, buffer_size);
            if (readChars == buffer_size) {
                if (strlen(_entry_subscription.topic_name) < MAXIMUM_TOPIC_NAME_LENGTH &&
                    (strcmp(_entry_subscription.topic_name, topic_name) == 0)) {
                    // already subscribed
#if PERSISTENT_DEBUG
                    char uint16_buf[6];
                    sprintf(uint16_buf, "%d", _entry_subscription.topic_id);
                    logger->append_log(" - ");
                    logger->append_log(uint16_buf);
#endif
                    return _entry_subscription.topic_id;

                }
            }
        } while (readChars > 0);
#if PERSISTENT_DEBUG
        logger->append_log(" - not subscribed");
#endif
        return 0;
    }





    virtual uint16_t get_client_await_msg_id() {
        if (!_transaction_started || _error) {
            return 0;
        }
        if (_not_in_client_registry) {
            return 0;
        }
        return _entry_client.await_message_id;
    }

    virtual void set_client_await_msg_id(uint16_t msg_id) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }
        _entry_client.await_message_id == msg_id;

        _open_file.close();
        _open_file = SD.open(client_registry, FILE_WRITE);
        int n = this->parse_file_number_to_int(&_entry_client);
        _open_file.seek(n * sizeof(entry_client));
        _open_file.write((const char *) &_entry_client, sizeof(entry_client));
    }



    virtual void get_last_client_address(device_address *target_address) {

        _open_file.close();
        _open_file = SD.open(client_registry, FILE_READ);
        // device_address empty_address;
        // memset(&empty_address, 0, sizeof(device_address));

        entry_client _entry_client;
        uint32_t line_number = 0;
        int readChars = 0;
        do {
            memset(&_entry_client, 0, sizeof(entry_client));
            uint16_t buffer_size = sizeof(entry_client);

            readChars = _open_file.read(&_entry_client, buffer_size);
            if (readChars == buffer_size && _entry_client.client_status != EMPTY) {
                memcpy(target_address, &_entry_client.client_address, sizeof(device_address));
            }
        } while (readChars > 0);
        _open_file.close();
    }


    void
    get_nth_client(uint64_t n, char *client_id, device_address *target_address, CLIENT_STATUS *target_status,
                   uint32_t *target_duration,
                   uint32_t *target_timeout) {

        _open_file.close();
        _open_file = SD.open(client_registry, FILE_READ);
        _open_file.seek(n * sizeof(entry_client));
        entry_client _entry_client;
        memset(&_entry_client, 0, sizeof(entry_client));
        int buffer_size = sizeof(entry_client);
        int readChars = _open_file.read(&_entry_client, buffer_size);
        memset(client_id, 0, MAXIMUM_CLIENT_ID_LENGTH);

        memcpy(target_address, &_entry_client.client_address, sizeof(device_address));
        strcpy(client_id, _entry_client.client_id);
        *target_status = _entry_client.client_status;
        *target_duration = _entry_client.duration;
        *target_timeout = _entry_client.timeout;

    }

    virtual void set_timeout(uint32_t timeout) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }
        _open_file.close();
        _open_file = SD.open(client_registry, FILE_WRITE);
        int client_position = parse_file_number_to_int(&_entry_client);
        _entry_client.timeout = timeout;
        _open_file.seek(client_position * sizeof(entry_client));
        _open_file.write((const char *) &_entry_client, sizeof(entry_client));
        _open_file.close();
    }

    virtual void set_client_state(CLIENT_STATUS status) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }

        _open_file.close();
        _open_file = SD.open(client_registry, FILE_WRITE);

        _entry_client.client_status = status;
        int client_position = parse_file_number_to_int(&_entry_client);
        _open_file.seek(client_position * sizeof(entry_client));
        _open_file.write((const char *) &_entry_client, sizeof(entry_client));
        _open_file.close();
    }

    virtual bool has_client_will() {
        if (!_transaction_started || _error) {
            return false;
        }
        if (_not_in_client_registry) {
            return false;
        }

        _open_file.close();

        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(WILL_FILE_ENDING)];

        // will file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(WILL_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], WILL_FILE_ENDING,
               strlen(WILL_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);

        entry_will _entry_will;
        int readChar = _open_file.read(&_entry_will, sizeof(entry_will));
        return readChar == sizeof(entry_will);
    }

    virtual void get_client_will(char *target_willtopic, uint8_t *target_willmsg, uint8_t *target_willmsg_length,
                                 uint8_t *target_qos, bool *target_retain) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }
        _open_file.close();

        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(WILL_FILE_ENDING)];
        // will file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(WILL_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], WILL_FILE_ENDING,
               strlen(WILL_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);

        entry_will _entry_will;
        int readChar = _open_file.read(&_entry_will, sizeof(entry_will));
        if (readChar == sizeof(entry_will)) {
            strcpy(target_willtopic, _entry_will.willtopic);
            memcpy(target_willmsg, _entry_will.willmsg, _entry_will.willmsg_length);
            *target_willmsg_length = _entry_will.willmsg_length;

            *target_qos = _entry_will.qos;
            *target_retain = _entry_will.retain;
        }
    }

    virtual void set_client_willtopic(char *willtopic, uint8_t qos, bool retain) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }

        if (strlen(willtopic) >= MAXIMUM_TOPIC_NAME_LENGTH) {
            return;
        }
        _open_file.close();
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(WILL_FILE_ENDING)];
        // will file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(WILL_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], WILL_FILE_ENDING,
               strlen(WILL_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);

        entry_will _entry_will;
        memset(&_entry_will, 0, sizeof(_entry_will));

        int readChars = 0;
        int buffer_size = sizeof(entry_will);
        readChars = _open_file.read(&_entry_will, buffer_size);

        memset(&_entry_will.willtopic, 0, sizeof(_entry_will.willtopic));
        strcpy(_entry_will.willtopic, willtopic);
        _entry_will.qos = qos;
        _entry_will.retain = retain;


        _open_file.close();
        _open_file = SD.open(filename_with_extension, FILE_WRITE);
        _open_file.seek(0);
        _open_file.write((uint8_t *) &_entry_will, buffer_size);

    }

    virtual void set_client_willmessage(uint8_t *willmsg, uint8_t willmsg_length) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }
        _open_file.close();
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(WILL_FILE_ENDING)];
        // will file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(WILL_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], WILL_FILE_ENDING,
               strlen(WILL_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);

        entry_will _entry_will;
        memset(&_entry_will, 0, sizeof(_entry_will));

        int readChars = 0;
        int buffer_size = sizeof(entry_will);
        readChars = _open_file.read(&_entry_will, buffer_size);

        memset(&_entry_will.willmsg, 0, sizeof(_entry_will.willmsg));
        memcpy(&_entry_will.willmsg, willmsg, willmsg_length);
        _entry_will.willmsg_length = willmsg_length;

        _open_file.close();
        _open_file = SD.open(filename_with_extension, FILE_WRITE);
        _open_file.seek(0);
        _open_file.write((uint8_t *) &_entry_will, buffer_size);
    }


    virtual message_type get_client_await_message_type() {
        if (!_transaction_started || _error) {
            return MQTTSN_PINGREQ;
        }
        if (_not_in_client_registry) {
            return MQTTSN_PINGREQ;
        }
        return _entry_client.await_message;
    }


    // TODO ordered until here

    virtual uint32_t get_global_topic_subscription_count(const char *topic_name) {
        if (_error) {
            return 0;
        }
        if (topic_name == nullptr || strlen(topic_name) == 0 || strlen(topic_name) >= MAXIMUM_TOPIC_NAME_LENGTH) {
            return 0;
        }

        if (strlen(topic_name) > MAXIMUM_TOPIC_NAME_LENGTH) {
            _error = true;
            return 0;
        }
        _open_file.close();

        _open_file = SD.open(mqtt_sub, FILE_READ);

#if PERSISTENT_DEBUG
        logger->start_log("get_topic_subscription_count ", 3);
        logger->append_log(topic_name);
#endif

        entry_mqtt_subscription _entry_mqtt_subscription;
        uint16_t entry_number = 0;
        int readChars = 0;
        int32_t first_empty_space = -1;
        do {
            memset(&_entry_mqtt_subscription, 0, sizeof(entry_mqtt_subscription));
            uint16_t buffer_size = sizeof(entry_mqtt_subscription);
            readChars = _open_file.read((char *) &_entry_mqtt_subscription, buffer_size);
            if (readChars == buffer_size) {
                if (_entry_mqtt_subscription.client_subscription_count != 0 &&
                    strlen(_entry_mqtt_subscription.topic_name) < MAXIMUM_TOPIC_NAME_LENGTH &&
                    (strcmp(_entry_mqtt_subscription.topic_name, topic_name) == 0)) {
                    // found entry
#if PERSISTENT_DEBUG
                    logger->append_log(" exists - count ");
                    char uint16_buf[6];
                    sprintf(uint16_buf, "%d", _entry_mqtt_subscription.client_subscription_count);
                    logger->append_log(uint16_buf);
#endif
                    return _entry_mqtt_subscription.client_subscription_count;
                }
            } else if (readChars != 0 && readChars < buffer_size) {
                break;
            }
            entry_number++;
        } while (readChars > 0);
#if PERSISTENT_DEBUG
        logger->append_log(" not exist");
#endif
        return 0;
    }

    // publish

    virtual bool has_client_publishes() {
        if (!_transaction_started || _error) {
            return false;
        }
        if (_not_in_client_registry) {
            return false;
        }

        _open_file.close();
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(PUBLISH_FILE_ENDING)];
        // will file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(PUBLISH_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], PUBLISH_FILE_ENDING,
               strlen(PUBLISH_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);

        entry_publish _entry_publish;
        int readChars = 0;
        do {
            memset(&_entry_publish, 0, sizeof(entry_publish));
            uint16_t buffer_size = sizeof(entry_publish);
            readChars = _open_file.read((char *) &_entry_publish, buffer_size);
            if (readChars == buffer_size) {
                if (_entry_publish.publish_id != 0) {
                    // found a non empty publish
                    return true;
                }

            } else if (readChars != 0 && readChars < buffer_size) {
                break;
            }
        } while (readChars > 0);

        return false;
    }


    virtual void add_client_publish(uint8_t *data, uint8_t data_len, uint16_t topic_id, bool retain,
                                    uint8_t qos, bool dup, uint16_t msg_id) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }

        _open_file.close();
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(PUBLISH_FILE_ENDING)];
        // will file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(PUBLISH_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], PUBLISH_FILE_ENDING,
               strlen(PUBLISH_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);


        entry_publish _entry_publish;
        uint16_t  entry_number = 0;
        int readChars = 0;
        int32_t first_empty_space = -1;
        do {
            memset(&_entry_publish, 0, sizeof(entry_publish));
            uint16_t buffer_size = sizeof(entry_publish);
            readChars = _open_file.read((char *) &_entry_publish, buffer_size);
            if (readChars == buffer_size) {
                if (_entry_publish.publish_id == 0) {
                    first_empty_space = entry_number;
                }

            } else if (readChars != 0 && readChars < buffer_size) {
                break;
            }
            entry_number++;
        } while (readChars > 0);

        if (first_empty_space == -1) {
            // no empty space => append
            first_empty_space = entry_number-1;
        }

        memset(&_entry_publish, 0, sizeof(entry_publish));
        memcpy(&_entry_publish.msg, data, data_len);
        _entry_publish.msg_length = data_len;
        _entry_publish.topic_id = topic_id;
        _entry_publish.qos = qos;
        _entry_publish.retain = retain;
        _entry_publish.dup = false;
        _entry_publish.msg_id = 0;
        // TODO fix type stuff
        _entry_publish.publish_id = first_empty_space+1;
        _entry_publish.retransmition_timeout = 0;

        _open_file.close();
        _open_file = SD.open(filename_with_extension, FILE_WRITE);
        _open_file.seek(first_empty_space * sizeof(entry_publish));
        _open_file.write((uint8_t *) &_entry_publish, sizeof(entry_publish));
    }


    virtual void
    get_next_publish(uint8_t *data, uint8_t *data_len, uint16_t *topic_id, bool *retain,
                     uint8_t *qos, bool *dup, uint16_t *publish_id) {
        if (!_transaction_started || _error) {
            *data_len = 0;
            *publish_id = 0;
            return;
        }
        if (_not_in_client_registry) {
            *data_len = 0;
            *publish_id = 0;
            return;
        }

        _open_file.close();
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(PUBLISH_FILE_ENDING)];
        // will file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(PUBLISH_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], PUBLISH_FILE_ENDING,
               strlen(PUBLISH_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);


        entry_publish _entry_publish;
        uint16_t  entry_number = 0;
        int readChars = 0;
        int32_t last_publish_entry = -1;
        do {
            memset(&_entry_publish, 0, sizeof(entry_publish));
            uint16_t buffer_size = sizeof(entry_publish);
            readChars = _open_file.read((char *) &_entry_publish, buffer_size);
            if (readChars == buffer_size) {
                if (_entry_publish.publish_id != 0) {
                    last_publish_entry = entry_number;
                }

            } else if (readChars != 0 && readChars < buffer_size) {
                break;
            }
            entry_number++;
        } while (readChars > 0);

        if (last_publish_entry == -1) {
            // no empty publish available
            *data_len = 0;
            *publish_id = 0;
            return;
        }

        _open_file.close();
        _open_file = SD.open(filename_with_extension, FILE_READ);
        _open_file.seek(last_publish_entry * sizeof(entry_publish));
        memset(&_entry_publish, 0, sizeof(entry_publish));
        uint16_t buffer_size = sizeof(entry_publish);
        _open_file.read((char *) &_entry_publish, buffer_size);

        memcpy(data, &_entry_publish.msg, _entry_publish.msg_length);
        *data_len = _entry_publish.msg_length;
        *topic_id = _entry_publish.topic_id,
        *retain = _entry_publish.retain;
        *qos = _entry_publish.qos;
        *dup = _entry_publish.dup;
        *publish_id = _entry_publish.publish_id;
    }


    virtual void set_publish_msg_id(uint16_t publish_id, uint16_t msg_id) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }

        _open_file.close();
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(PUBLISH_FILE_ENDING)];
        // will file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(PUBLISH_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], PUBLISH_FILE_ENDING,
               strlen(PUBLISH_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);
        _open_file.seek((publish_id-1) * sizeof(entry_publish));

        entry_publish _entry_publish;
        int readChars = 0;
        memset(&_entry_publish, 0, sizeof(entry_publish));
        uint16_t buffer_size = sizeof(entry_publish);
        readChars = _open_file.read((char *) &_entry_publish, buffer_size);
        if (readChars == buffer_size) {
            if (_entry_publish.publish_id == publish_id) {
                _entry_publish.msg_id = msg_id;
                _open_file.close();
                _open_file = SD.open(filename_with_extension, FILE_WRITE);
                _open_file.seek((publish_id-1) * sizeof(entry_publish));
                _open_file.write((uint8_t *) &_entry_publish, sizeof(entry_publish));
            }else{
                _error = true;
            }
        }
    }



    virtual void remove_publish_by_msg_id(uint16_t msg_id) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }
        if (msg_id == 0) {
            // there is no message id which is zero (0)
            _error = true;
        }

        _open_file.close();
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(PUBLISH_FILE_ENDING)];
        // will file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(PUBLISH_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], PUBLISH_FILE_ENDING,
               strlen(PUBLISH_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);


        entry_publish _entry_publish;
        uint16_t  entry_number = 0;
        int readChars = 0;
        int32_t last_publish_entry = -1;
        do {
            memset(&_entry_publish, 0, sizeof(entry_publish));
            uint16_t buffer_size = sizeof(entry_publish);
            readChars = _open_file.read((char *) &_entry_publish, buffer_size);
            if (readChars == buffer_size) {
                if (_entry_publish.msg_id == msg_id) {
                    // found
                    memset(&_entry_publish, 0, sizeof(entry_publish));
                    _open_file = SD.open(filename_with_extension, FILE_WRITE);
                    _open_file.seek(entry_number * sizeof(buffer_size));
                    _open_file.write((char *) &_entry_publish, sizeof(buffer_size));
                    return;
                }

            } else if (readChars != 0 && readChars < buffer_size) {
                break;
            }
            entry_number++;
        } while (readChars > 0);
        // there is no message id with the give msg_id
    }


    virtual void remove_publish_by_publish_id(uint16_t publish_id) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }

        _open_file.close();
        char filename_with_extension[sizeof(_entry_client.file_number) + sizeof(PUBLISH_FILE_ENDING)];
        // will file
        memset(&filename_with_extension, 0, sizeof(_entry_client.file_number) + sizeof(PUBLISH_FILE_ENDING));
        memcpy(&filename_with_extension, &_entry_client.file_number, strlen(_entry_client.file_number));
        memcpy(&filename_with_extension[strlen(_entry_client.file_number)], PUBLISH_FILE_ENDING,
               strlen(PUBLISH_FILE_ENDING));
        _open_file = SD.open(filename_with_extension, FILE_READ);
        _open_file.seek((publish_id-1) * sizeof(entry_publish));

        entry_publish _entry_publish;
        int readChars = 0;
        memset(&_entry_publish, 0, sizeof(entry_publish));
        uint16_t buffer_size = sizeof(entry_publish);
        readChars = _open_file.read((char *) &_entry_publish, buffer_size);
        if (readChars == buffer_size) {
            if (_entry_publish.publish_id == publish_id) {
                memset(&_entry_publish, 0, sizeof(entry_publish));
                _open_file.close();
                _open_file = SD.open(filename_with_extension, FILE_WRITE);
                _open_file.seek((publish_id-1) * sizeof(entry_publish));
                _open_file.write((uint8_t *) &_entry_publish, sizeof(entry_publish));
            }else{
                _error = true;
            }
        }
    }

    // gateway configuration

    virtual uint16_t get_advertise_duration() {
        return 900;
    }

    virtual bool get_gateway_id(uint8_t *gateway_id) {
        return 2;
    }


    virtual bool get_mqtt_config(uint8_t *server_ip, uint16_t *server_port, char *client_id) {

#if PERSISTENT_DEBUG
        logger->log("loading Mqtt configuration", 2);
#endif
        //_open_file.close();
        _open_file = SD.open(mqtt_configuration);


        const char *b_addr = "brokeraddress";
        const char *b_port = "brokerport";
        const char *c_id = "clientid";
        bool has_b_addr = false, has_b_port = false, has_c_id = false;
        char buffer[512];
        memset(&buffer, 0, sizeof(buffer));
        while (readLine((char *) &buffer, sizeof(buffer)) > 0) {
            uint16_t line_length = (uint16_t) (strlen(buffer) + 1);
            if (memcmp(&buffer, b_addr, strlen(b_addr)) == 0) {
                has_b_addr = parse_ipaddress_after_space(server_ip, buffer, line_length);
            } else if (memcmp(&buffer, b_port, strlen(b_port)) == 0) {
                has_b_port = parse_uint16_t_after_space(server_port, buffer, line_length);
            } else if (memcmp(&buffer, c_id, strlen(c_id)) == 0) {
                has_c_id = parse_string_after_space(client_id, buffer, line_length);
            }
            memset(&buffer, 0, sizeof(buffer));
        }
#if PERSISTENT_DEBUG
        if (has_b_addr && has_b_port && has_c_id) {
            logger->log("Mqtt configuration loaded", 2);
        } else {
            logger->start_log("Mqtt configuration incomplete missing: ", 2);
            if (!has_b_addr) {
                logger->append_log(" brokeraddress");
            }
            if (!has_b_port) {
                logger->append_log(" brokerport");
            }
            if (!has_c_id) {
                logger->append_log(" clientid");
            }
        }
#endif
        _open_file.close();
        return has_b_addr && has_b_port && has_c_id;

    }


    virtual bool get_mqtt_login_config(char *username, char *password) {

        _open_file.close();
        _open_file = SD.open(mqtt_configuration, FILE_READ);

        const char *c_username = "username";
        const char *c_password = "password";
        bool has_c_username = false, has_c_password = false;
        char buffer[128];
        memset(&buffer, 0, sizeof(buffer));
        while (readLine((char *) &buffer, sizeof(buffer)) > 0) {
            uint16_t line_length = (uint16_t) (strlen(buffer) + 1);
            if (memcmp(&buffer, c_username, strlen(c_username)) == 0) {
                has_c_username = parse_string_after_space(username, buffer, line_length);
            } else if (memcmp(&buffer, c_password, strlen(c_password)) == 0) {
                has_c_password = parse_string_after_space(password, buffer, line_length);
            }

            memset(&buffer, 0, sizeof(buffer));
        }
        return has_c_username && has_c_password;
    }

    virtual bool get_mqtt_will(char *will_topic, char *will_msg, uint8_t *will_qos, bool *will_retain) {
#if PERSISTENT_DEBUG
        logger->log("loading Mqtt will", 2);
#endif
        _open_file.close();
        _open_file = SD.open(mqtt_configuration, FILE_READ);

        const char *w_topic = "willtopic";
        const char *w_msg = "willmessage";
        const char *w_qos = "willqos";
        const char *w_retain = "willretain";
        bool has_w_topic = false, has_w_msg = false;
        bool has_w_qos = false, has_w_retain = false;
        char buffer[128];
        memset(&buffer, 0, sizeof(buffer));
        while (readLine((char *) &buffer, sizeof(buffer)) > 0) {
            uint16_t line_length = (uint16_t) (strlen(buffer) + 1);
            if (memcmp(&buffer, w_topic, strlen(w_topic)) == 0) {
                has_w_topic = parse_string_after_space(will_topic, buffer, line_length);
            } else if (memcmp(&buffer, w_msg, strlen(w_msg)) == 0) {
                has_w_msg = parse_string_after_space(will_msg, buffer, line_length);
            } else if (memcmp(&buffer, w_qos, strlen(w_qos)) == 0) {
                has_w_qos = parse_uint8_t_after_space(will_qos, buffer, line_length);
            } else if (memcmp(&buffer, w_retain, strlen(w_retain)) == 0) {
                uint8_t tmp_number = 0;
                bool parse_success = parse_uint8_t_after_space(&tmp_number, buffer, line_length);
                if (parse_success && tmp_number <= 1) {
                    *will_retain = tmp_number != 0;
                    has_w_retain = true;
                }
            }
            memset(&buffer, 0, sizeof(buffer));
        }

#if PERSISTENT_DEBUG
        if (has_w_topic && has_w_msg && has_w_qos && has_w_retain) {
            logger->log("Mqtt will loaded", 2);
        } else {
            logger->start_log("Mqtt will incomplete missing: ", 2);
            if (!has_w_topic) {
                logger->append_log(" willtopic");
            }
            if (!has_w_msg) {
                logger->append_log(" willmessage");
            }
            if (!has_w_qos) {
                logger->append_log(" willqos");
            }
            if (!has_w_retain) {
                logger->append_log(" willretain");
            }
        }
#endif
        return has_w_topic && has_w_msg && has_w_qos && has_w_retain;
    }

    // getway connection status
    bool _is_mqttsn_online = false;
    bool _is_mqtt_online = false;


    virtual uint8_t set_mqttsn_disconnected() {
#if PERSISTENT_DEBUG
        logger->log("Socket error on MQTTSN",0);
#endif
        _is_mqttsn_online = false;
        return SUCCESS;
    }

    virtual uint8_t set_mqtt_disconnected() {
#if PERSISTENT_DEBUG
        logger->log("Socket error on MQTT",0);
#endif
        _is_mqtt_online = false;
        return SUCCESS;
    }

    virtual uint8_t set_mqttsn_connected() {
        _is_mqttsn_online = true;
        return SUCCESS;
    }

    virtual uint8_t set_mqtt_connected() {
        _is_mqtt_online = true;
        return SUCCESS;
    }

    virtual bool is_mqttsn_online() {
        return _is_mqttsn_online;
    }

    virtual bool is_mqtt_online() {
        return _is_mqtt_online;
    }


    virtual void get_client_id(char *client_id) {
        if (!_transaction_started || _error) {
            return;
        }
        if (_not_in_client_registry) {
            return;
        }
        if (strlen(_entry_client.client_id) >= MAXIMUM_CLIENT_ID_LENGTH) {
            return;
        }
        strcpy(client_id, (char *) _entry_client.client_id);
    }

protected:

    virtual int parse_file_number_to_int(entry_client *_entry_client) {
        //int empty_space = atoit(_entry_client.file_number);
        //int client_position = strtol(_entry_client.file_number, &_entry_client.file_number, 10);
        // we have to use ataoi:
        // https://github.com/esp8266/Arduino/issues/404
        // http://www.esp8266.com/viewtopic.php?f=28&t=4001
        return atoi(_entry_client->file_number);
    }


};

#endif //GATEWAY_PERSISTENTIMPL_H
