#ifndef ESPARDUINOMQTTSNGATEWAY_ARDUINOLOGGER_H
#define ESPARDUINOMQTTSNGATEWAY_ARDUINOLOGGER_H

#include "../LoggerInterface.h"
#include "Arduino.h"
#include <chrono>


class ArduinoLogger : public LoggerInterface {
private:
    uint8_t current_log_lvl = 2;
    uint8_t last_started_log_lvl = UINT8_MAX;
    SerialMock Serial;

public:

    /**
     * Initializes logger
     * @return if logger is successfully initialized or not
     */
    virtual bool begin() override;

    /**
      * Set the logging level only message with log_lvl smaller or equal then the set logging level will be logged.
      * The default level is 2, only message with log_lvl 2,1 and 0 are printed.
      * Note:
     * Logging level shall indicate how detailed the loggin output is. Higher means more detailled.
     * A logging level 0 shall only be used for fatal errors inside the Gateway and will be always logged.
     * @param log_lvl
     */
    void set_log_lvl(uint8_t log_lvl) override;


    /**
     * logs a single line with timestamp or whatever at the beginning.
     * @param msg to be logged
     */
    void log(char *msg, uint8_t log_lvl) override ;

    /**
     * logs a single line with timestamp or whatever at the beginning.
     * @param msg to be logged
     */
    void log(const char *msg, uint8_t log_lvl) override;

    /**
     * logs a line can be appended with append_log
     * @param msg
     */
    void start_log(char *msg, uint8_t log_lvl) override;

    /**
     * logs a line can be appended with append_log
     * @param msg
     */
    void start_log(const char *msg, uint8_t log_lvl) override;


    /**
     * sets the log lvl for log message to be printed
     * @param msg
     */
    void set_current_log_lvl(uint8_t log_lvl) override;

    /**
     * append message to the last log message
     * @param msg to be appended
     */
    void append_log(char *msg) override;

    /**
     * append message to the last log message
     * @param msg to be appended
     */
    void append_log(const char *msg) override;

};


#endif //ESPARDUINOMQTTSNGATEWAY_ARDUINOLOGGER_H
