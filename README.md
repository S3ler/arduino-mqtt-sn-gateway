# arduino-mqtt-sn-gateway

MQTT-SN gateway for arduino based microntrollers and linux based operating systems.

Note about the current status of this project:
It is completely work in progress and only partly tested.
It runs only on Linux system but it is written against the Arduino Framework by using Linux based fake implementations for Arduino functions and libraris.
It runs (successfully) on a [NodeMCU](http://nodemcu.com/index_en.html) with [Arduino Core](https://github.com/esp8266/Arduino) and a SPI SD-Card reader.

This implementation is a aggregating gateway for [MQTT-SN](http://mqtt.org/new/wp-content/uploads/2009/06/MQTT-SN_spec_v1.2.pdf).
The aim of MQTT-SN is to support Wireless Sensor Networks (WSNs) with very large numbers of battery-operated sensors and actuators (SAs).
This gateway is designed with platform independence in mind. Other MQTT-SN gateway implementation at least need a operating system (or only bridge messages).
To fulfill the requierement the implementation shall use little RAM, no dynamic memory allocation, platform and hardware portability by interfaces.
Further it does not rely on any physical layer, media access control (e.g IEEE 802.15.4), network layer (e.g. 6LoWPAN) or transport layer (e.g. UDP/TCP) standard or implementation. It is designed to run on as many platforms (and microntrollers) as possible especially Arduinos [Arduino](https://www.arduino.cc/en/Reference/HomePage).
For needed functionality for implementations on other platform and operating system see the implementation notes and porting to other systems section.


```
    (Device)_____                          ____________________            _______________
                  \___________            |                    |          |               |
                      MQTT-SN \__ ________|                    |   MQTT   |               |
          (Device)________________________|      Gateway       |__________|  MQTT Broker  |
                               __ ________|  (mqtt-sn gateway) |          |               |
            __________________/           |                    |          |               |
    (Device)                               --------------------            ---------------
                                                                                  |
                                                                             MQTT |
                                                                           _______|________
                                                                          |               |
                                                                          |  Other MQTT   |
                                                                          |    Devices    |
                                                                           ---------------
```

## quick start (running the gateway)
This is the section for all of you who only want to use the gateway.
The gateway need the following configurations files: MQTT.CON, TOPICS.PRE

The MQTT.CON file is the configuration file of the MQTT client. Values are space separated.

The following values are NOT optional:

  * brokeraddress - IP address of the MQTT Broker, DNS or mDNS are not implemented but on the TODO list.
  * brokerport - Port of the MQTT Broker
  * clientid - MQTT client's id
  * gatewayid - Id of the gateway in the WSN

You can provide a will for the gateway (optional):

  * willtopic - topic of the will message
  * willmessage - payload of the willmessage (only ASCII supported)
  * willqos - quality of service of the will
  * willretain - retain of the will

Note that you need to provide all values of the will (there are no default values) or the gateway will connect without a will.

Example MQTT.CON file:

	brokeraddress 192.168.178.33
	brokerport 1884
	clientid mqtt-sn linux gateway
	willtopic /mqtt/sn/gateway
	willmessage /mqtt-sn linux gateway offline
	willqos 1
	willretain 0
	gatewayid 2

The TOPCIS.PRE file is the list of predefined MQTT topics of the gateway. Entries are space separated.
Each entry starts with the topic id followed by the topic name.
If a topic id is not unqiue in the file, the first topic if found by the gateway (starting at the beginning of the file) will be used.

Example TOPICS.PRE file:

	50 /some/predefined/topic
	20 /another/predefined/topic

## implementation notes
This MQTT-SN gateway is a aggregating gateway implementation. Although a transparent gateway implementation is simpler a transparent solution needs a separation TCP connection for each MQTT-SN client. Resource constrained devices cannot handle the number of concurrent connections. The gateway has only one TCP connection to the MQTT Broker meaning it appears as a single MQTT Client on the MQTT Broker.

TODO core class diagram

TODO linux with arduino class diagram

## porting to other systems
When you want to use this gateway on another platform you need to provide some functionality to the gateway core.
The gateway core uses several interfaces for hardware and operating system abstraction.
The requierements are:
- XXX kB RAM
- C/C++ compiler for your platform
- C/C++ based MQTT client implementation
- implementations of the interfaces (socket, system, persistence mqtt message handler) shown in care class diagram
