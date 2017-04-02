//
// Created by bele on 23.12.16.
//

#ifndef GATEWAY_UDPSOCKETIMPL_H
#define GATEWAY_UDPSOCKETIMPL_H

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <ifaddrs.h>
#include "../SocketInterface.h"

#define BUFLEN 255	//Max length of buffer
#define PORT 8888	//The port on which to listen for incoming data

/**
 * This is a example implementation for the SocketInterface based on Linux UDP Sockets.
 * This can be used as a reference implementation.
 */
class UdpSocketImpl : public SocketInterface{
public:
    struct sockaddr_in si_me, si_other;

    int s, i, recv_len;
    socklen_t slen = sizeof(si_other);
    char buf[BUFLEN];

    MqttSnMessageHandler *mqttsn = nullptr;
    device_address own_address;
    device_address broadcast_address;

    LoggerInterface *logger;
public:
    bool begin();

    void setMqttSnMessageHandler(MqttSnMessageHandler *mqttSnMessageHandler) override;

    void setLogger(LoggerInterface *logger) override;

    device_address *getBroadcastAddress() override;

    device_address *getAddress() override;

    uint8_t getMaximumMessageLength() override;

    bool send(device_address *destination, uint8_t *bytes, uint16_t bytes_len) override;

    bool send(device_address *destination, uint8_t *bytes, uint16_t bytes_len, uint8_t signal_strength) override;

    bool loop() override;

    device_address getDevice_address(sockaddr_in *addr) const;

    uint32_t getIp_address(device_address *address) const;

    uint16_t getPort(device_address *address) const;

    void disconnect();


};


#endif //GATEWAY_UDPSOCKETIMPL_H
