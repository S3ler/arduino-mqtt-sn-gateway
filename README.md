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

## Getting Started



## Build Your Gateway
TODO for each Gateway describe the following steps
### Arduino Mega UDP & TCP Gateway
#### Components

#### Wiring

#### Programming

#### Configuration

## Development
TODO describe how to start the development


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


## Supported Platforms and Technologies
We try to provide implementations for a wide range of transmission technologies and platforms.
Depend on the stage of project you have already chosen your technologies and platforms, then jump to How to build the gateway.

If not you may want to choose a platform and transmission technology first, by using the platform-transmition-technology chart:

                      | Ethernet            | WiFi     | Bluetooth LE | ZigBee      | LoRa                        |
    ESP8266 (Arduino) | untested            | yes      | untested     | untested    | untested                    |
    Module            | Ethernet Shield V1  | SoC      | nRF51822     | XBee ZB S2C | SX1272 LoRa or Lora Shield  |
    Arduino MEGA 2560 | yes                 | unknown  | untested     | untested    | untested                    |
    Moduel            | Ethernet Shield V1  |          | nRF51822     | XBee ZB S2C | SX1272 LoRa or Lora Shield  |

(please note: the chart show where the gateway can be executed with, mqtt-sn-client has it's a own project)
unknown means: not tested or implemented, and most likely i will not implemented it due to missing hardware (e.g. too expensive)
untested means: not implemented yet, but most likely to come in the future ( can also provide a implementation for me )


## Purchasing and Test Prototyping Hardware
Here we describe sources for purchasing prototyping hardware and sources for testing the hardware.

### ESP8266
We suggest a **NodeMCU board**, it provides access to all IO Pins of the ESP.
The benefit when using the ESP8266 is that we have a powerfull SoC with more than enough RAM and processing power for the mqtt-sn-gateway. It only lacks in having a **SD Card Slot** ( [Amazon]( https://www.amazon.de/s/ref=nb_sb_noss?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&url=search-alias%3Daps&field-keywords=Arduino+SD+Card ), [AliExpress]( https://de.aliexpress.com/wholesale?catId=0&initiative_id=SB_20170515133800&SearchText=Arduino+SD+Card ) ) so get one for cheap money and a **SDHC Card** ( [Amazon]( https://www.amazon.de/s/ref=nb_sb_noss_2?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&url=search-alias%3Daps&field-keywords=SDHC+card&rh=i%3Aaps%2Ck%3ASDHC+card ), [AliExpress]( ) )
Purchase at: [Amazon]( https://www.amazon.de/s/ref=nb_sb_noss?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&url=search-alias%3Dcomputers&field-keywords=NodeMCU ) or [AliExpress]( https://de.aliexpress.com/wholesale?catId=0&initiative_id=SB_20170515132221&SearchText=NodeMcu ).
For programming can use the [Arduino IDE](https://www.arduino.cc/en/main/software) with [ESP8266 core for Arduino](https://github.com/esp8266/Arduino), or use [platformIO](http://platformio.org/).

### Arduino Mega
You can use any **Arduino MEGA 2560** (or **compatible boards** from SainSMart,Elegoo or SODIAL).
The Arduino MEGA 2560 has no SD Card Slot but in most cases you use a [ArduinoWiFiShield](https://www.arduino.cc/en/Main/ArduinoWiFiShield) or a [EthernetShield](https://www.arduino.cc/en/Main/ArduinoEthernetShield) containing a SD Card Slot so get a **microSDHC Card** ( [Amazon]( https://www.amazon.de/s/ref=nb_sb_noss?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&url=search-alias%3Dcomputers&field-keywords=SD+Card&rh=n%3A340843031%2Ck%3ASD+Card ), [AliExpress]( https://de.aliexpress.com/wholesale?catId=0&initiative_id=SB_20170515134621&SearchText=microSDHC ) ).
If you use Shields or technologies without a SD Card Slot buy some cheap ones ( [Amazon]( https://www.amazon.de/s/ref=nb_sb_noss?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&url=search-alias%3Daps&field-keywords=Arduino+SD+Card ), [AliExpress]( https://de.aliexpress.com/wholesale?catId=0&initiative_id=SB_20170515133800&SearchText=Arduino+SD+Card ) ) .
Purchase at: [Amazon]( https://www.amazon.de/s/ref=nb_sb_noss?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&url=search-alias%3Dcomputers&field-keywords=Arduino+Mega ), [AliExpress]( https://de.aliexpress.com/wholesale?catId=0&initiative_id=SB_20170515133130&SearchText=Arduino+mega ).
For programming can use the [Arduino IDE](https://www.arduino.cc/en/main/software), or use [platformIO](http://platformio.org/).

### Arduino Ethernet Shield
Any Arduino [Ethernet Shield](https://www.arduino.cc/en/Main/ArduinoEthernetShield) (or compatible shield) can be used. Does not matter if V1 (Chip WS5100) or V2 (Chip WS5500).
Purchase at: [Amazon]( https://www.amazon.de/s/ref=nb_sb_noss_2?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&url=search-alias%3Dcomputers&field-keywords=Arduino+Ethernet+Shield&rh=n%3A340843031%2Ck%3AArduino+Ethernet+Shield ), [AliExpress]( https://de.aliexpress.com/wholesale?catId=0&initiative_id=SB_20170515135409&SearchText=Arduino+Ethernet+Shield ).
Do not forget to buy a microSDHC Card if not already done.
For programming can use the [Arduino IDE](https://www.arduino.cc/en/main/software), or use [platformIO](http://platformio.org/).

### ZigBee
TODO

### Lora
TODO

