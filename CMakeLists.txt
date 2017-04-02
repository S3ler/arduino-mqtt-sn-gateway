cmake_minimum_required(VERSION 3.6)
project(ESPLinuxGatewayPort)

set(CMAKE_CXX_STANDARD 11)

file(GLOB GLOBAL_SOURCES *)

include_directories(src/Implementation)
include_directories(src/Implementation/paho/pahomqttembeddedc/MQTTClient/src/linux)
include_directories(src/Implementation/paho/pahomqttembeddedc/MQTTClient/src/)
include_directories(src/Implementation/paho/pahomqttembeddedc/MQTTPacket/src)

add_subdirectory(src/Implementation/paho)

set(INTERFACE_FILES
        src/core_defines.h
        src/CoreImpl.cpp
        src/CoreImpl.h
        src/CoreInterface.h
        src/Gateway.cpp
        src/Gateway.h
        src/global_defines.h
        src/LoggerInterface.h
        src/MqttMessageHandlerInterface.cpp
        src/MqttMessageHandlerInterface.h
        src/mqttsn_messages.h
        src/MqttSnMessageHandler.cpp
        src/MqttSnMessageHandler.h
        src/PersistentInterface.cpp
        src/PersistentInterface.h
        src/SocketInterface.cpp
        src/SocketInterface.h

        src/Implementation/Arduino.cpp
        src/Implementation/Arduino.h

        src/Implementation/ArduinoLogger.cpp
        src/Implementation/ArduinoLogger.h

        src/Implementation/ArduinoSystem.cpp
        src/Implementation/ArduinoSystem.h

        src/Implementation/SDLinuxFake.cpp
        src/Implementation/SDLinuxFake.h

        src/Implementation/UdpSocketImpl.cpp
        src/Implementation/UdpSocketImpl.h

        src/Implementation/paho/PahoMqttMessageHandler.h
        src/Implementation/paho/PahoMqttMessageHandler.cpp
        )

file(GLOB PAHO_SOURCE_FILES
        src/Implementation/paho/pahomqttembeddedc/MQTTPacket/src/*.c
        src/Implementation/paho/pahomqttembeddedc/MQTTPacket/src/*.h
        src/Implementation/paho/pahomqttembeddedc/MQTTClient/src/*.cpp
        src/Implementation/paho/pahomqttembeddedc/MQTTClient/src/*.h
        src/Implementation/paho/pahomqttembeddedc/MQTTClient/src/linux/*.cpp
        src/Implementation/paho/pahomqttembeddedc/MQTTClient/src/linux/*.h
        )

set(SOURCE_FILES src/main.cpp ${GLOBAL_SOURCES} ${INTERFACE_FILES} ${PAHO_SOURCE_FILES})
add_executable(ESPLinuxGatewayPort ${SOURCE_FILES})