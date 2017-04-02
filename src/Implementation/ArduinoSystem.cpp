#include "ArduinoSystem.h"

void ArduinoSystem::set_heartbeat(uint32_t period) {
    this->heartbeat_period = period;
}

uint32_t ArduinoSystem::get_heartbeat() {
    return this->heartbeat_period;
}


bool ArduinoSystem::has_beaten() {
    uint32_t current = millis();
    if (current - heartbeat_current > heartbeat_period) {
        this->heartbeat_current = current;
        return true;
    }
    return false;
}

uint32_t ArduinoSystem::get_elapsed_time() {
    uint32_t current = millis();
    uint32_t elapsed_time = current - elapsed_current;
    elapsed_current = current;
    return elapsed_time;
}

void ArduinoSystem::sleep(uint32_t duration) {
    delay(duration);
}

void ArduinoSystem::exit() {
    throw std::exception();
#if defined(ESP8266)
    ESP.reset();
#endif

}