//
// Created by bele on 18.12.16.
//

#ifndef GATEWAY_SD_TABLE_ENTRIES_H
#define GATEWAY_SD_TABLE_ENTRIES_H

#include <cstring>
#include <cstdio>
#include "../global_defines.h"
#include "../mqttsn_messages.h"
#include "../core_defines.h"

struct entry_mqtt_subscription{
    uint32_t client_subscription_count;
    char topic_name[255];
};

struct entry_will {
    char willtopic[255];
    char willmsg[255];
    uint8_t willmsg_length;
    uint8_t qos;
    bool retain;
};

struct entry_subscription{
    uint16_t topic_id;
    uint8_t qos;
    char topic_name[255];

};

struct entry_registration{
    uint16_t topic_id;
    char topic_name[255];
    bool known;
};

struct entry_publish{
    uint8_t msg[255];
    uint8_t msg_length;
    uint16_t topic_id;
    uint8_t qos;
    bool retain;
    bool dup;
    uint16_t msg_id;
    uint16_t publish_id;
    uint32_t retransmition_timeout; // not used atm
};

//TODO remove pragma and test
#pragma pack(push, 1)
struct entry_client {
    char client_id[24];
    char file_number[9];
    device_address client_address;
    CLIENT_STATUS client_status;
    uint32_t duration; // changed
    uint32_t timeout;
    uint16_t await_message_id;
    message_type await_message;
};
#pragma pack(pop)

#endif //GATEWAY_SD_TABLE_ENTRIES_H
