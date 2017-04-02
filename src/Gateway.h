//
// Created by bele on 22.12.16.
//

#ifndef GATEWAY_GATEWAY_H
#define GATEWAY_GATEWAY_H


#include "PersistentInterface.h"
#include "CoreInterface.h"
#include "CoreImpl.h"

class Gateway {
public:
    bool initialized = false;
    PersistentInterface *persistentInterface = nullptr;
    CoreImpl coreInterface;
    MqttMessageHandlerInterface *mqttInterface = nullptr;
    MqttSnMessageHandler mqttsnInterface;
    SocketInterface *socketInterface = nullptr;
    LoggerInterface *logger;
    System *system;
public:

    bool begin() {
        if (persistentInterface != nullptr  &&
            mqttInterface != nullptr &&
                socketInterface != nullptr) {

            /* -- connect components -- */

            // Socket
            socketInterface->setMqttSnMessageHandler(&mqttsnInterface);
            socketInterface->setLogger(logger);

            // Mqtt-SN
            mqttsnInterface.setSocket(socketInterface);
            mqttsnInterface.setCore(&coreInterface);
            mqttsnInterface.setLogger(logger);

            // Mqtt
            mqttInterface->setCore(&coreInterface);
            mqttInterface->setLogger(logger);

            // Persistent
            persistentInterface->setCore(&coreInterface);
            persistentInterface->setLogger(logger);

            // Core
            coreInterface.setPersistent(persistentInterface);
            coreInterface.setMqttMessageHandler(mqttInterface);
            coreInterface.setMqttSnMessageHandler(&mqttsnInterface);
            coreInterface.setLogger(logger);
            coreInterface.setSystem(system);

            if (logger->begin()) {
                initialized = coreInterface.begin();
            }
            return initialized;
        } else {
            return false;
        }
    }

    void setPersistentInterface(PersistentInterface *persistentInterface) {
        Gateway::persistentInterface = persistentInterface;
    }

    void setMqttInterface(MqttMessageHandlerInterface *mqttInterface) {
        Gateway::mqttInterface = mqttInterface;
    }

    void setSocketInterface(SocketInterface *socketInterface) {
        Gateway::socketInterface = socketInterface;
    }

    void setLoggerInterface(LoggerInterface *logger){
        Gateway::logger = logger;
    }

    void setSystemInterface(System *system){
        Gateway::system = system;
    }


    void loop() {
        if (initialized) {
            mqttInterface->loop();
            socketInterface->loop();
            coreInterface.loop();
        }
    }



};


#endif //GATEWAY_GATEWAY_H
