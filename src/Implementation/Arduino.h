//
// Created by bele on 28.05.16.
//

#ifndef DATABASE_ARDUINO_H
#define DATABASE_ARDUINO_H

#include <iostream>
#include <stdint.h>
#include <string.h>
#include <bitset>

#if defined(ARDUINO)
#else
#ifndef DATABASE_ARDUINO_TIMERS_H
#define DATABASE_ARDUINO_TIMERS_H

#include <chrono>

extern void delay(uint64_t time);

extern int64_t millis();

extern void yield();

extern int64_t random(int64_t min, int64_t max);

extern int64_t random(int64_t min, int64_t max);

extern void randomSeed(uint16_t seed);

#endif // DATABASE_ARDUINO_TIMERS_H
#endif

class SerialMock {

public:
    void begin(uint64_t);

    void println();
    // char
    void print(const char toPrint);

    void println(const char toPrint);

    void print(const char *toPrint);

    void println(const char *toPrint);
};

#endif //DATABASE_ARDUINO_H
