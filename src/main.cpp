
#include <paho/PahoMqttMessageHandler.h>
#include <UdpSocketImpl.h>
#include "Gateway.h"
#include "Implementation/SDPersistentImpl.h"
#include "Implementation/ArduinoLogger.h"
#include "Implementation/ArduinoSystem.h"


Gateway gateway;
UdpSocketImpl udpSocket;
SDPersistentImpl persistent;

PahoMqttMessageHandler mqtt;
ArduinoLogger logger;
ArduinoSystem systemImpl;

std::string getexepath()
{
    char result[ PATH_MAX ];
    ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
    return std::string( result, (count > 0) ? count : 0 );
}

// TODOS:
// implement message saving
// implement resubscribing on startup

void setup() {
    logger.start_log("Linux MQTT-SN Gateway version 0.0.1a starting", 1);
    logger.append_log("Ethernet connected!");

    gateway.setLoggerInterface(&logger);
    gateway.setSocketInterface(&udpSocket);
    gateway.setMqttInterface(&mqtt);
    gateway.setPersistentInterface(&persistent);
    gateway.setSystemInterface(&systemImpl);

    while (!gateway.begin()) {
        logger.log("Error starting gateway components", 0);
        systemImpl.sleep(5000);
        systemImpl.exit();
    }
    logger.log("Gateway ready", 1);
}

int main(int argc, char* argv[]) {
    std::string workingDir = getexepath() + "/../DB";
    workingDir = "/home/bele/git/MQTT-SN_nRF24L01/ESPLinuxGatewayPort/cmake-build-debug/DB";
    persistent.setRootPath((char *) workingDir.c_str());
    setup();
    while(true){
        gateway.loop();
    }
}



