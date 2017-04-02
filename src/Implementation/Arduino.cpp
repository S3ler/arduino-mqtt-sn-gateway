#include <termios.h>
#include <fcntl.h>
#include "Arduino.h"

void SerialMock::begin(uint64_t) {

}

void SerialMock::println() {
    std::cout << std::endl;
}

void SerialMock::print(const char toPrint) {
    std::cout << toPrint<< std::flush;
}

void SerialMock::println(const char toPrint) {
    std::cout << toPrint << std::endl;
}

void SerialMock::print(const char *toPrint) {
    std::cout << toPrint << std::flush;
}

void  SerialMock::println(const char *toPrint) {
    std::cout << toPrint << std::endl;
}

// DATABASE_ARDUINO_TIMERS_H
void delay(uint64_t time) {
    struct timespec tim;
    uint64_t seconds = (uint64_t) time / 1000;
    tim.tv_sec = seconds;
    uint64_t nseconds = (time - seconds * 1000) * (uint64_t) 1000000000L;
    tim.tv_nsec = nseconds;
    nanosleep(&tim, (struct timespec *) NULL);
    return;
}

int64_t timerValue = 0;
bool timerValueCalled = false;

int64_t millis() {
    if (!timerValueCalled) {
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
        );
        timerValue = ms.count();
        timerValueCalled = true;
        return 0;
    }
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
    );
    int64_t diff = ms.count() - timerValue;
    if (diff > UINT32_MAX) {
        // overflow
        timerValue = diff - UINT32_MAX;
        return timerValue;
    }
    return diff;
}

void yield() { }

int64_t random(int64_t min, int64_t max) {
    return rand() % max + min;
}

void randomSeed(uint16_t seed) {

}

