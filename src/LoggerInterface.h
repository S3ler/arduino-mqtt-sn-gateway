//
// Created by bele on 06.01.17.
//

#ifndef ESPARDUINOMQTTSNGATEWAY_LOGGERINTERFACE_H
#define ESPARDUINOMQTTSNGATEWAY_LOGGERINTERFACE_H

#include <stdint.h>

class LoggerInterface {

public:

    virtual ~LoggerInterface() {};

    /**
     * Initializes logger
     * @return if logger is successfully initialized or not
     */
    virtual bool begin()=0;

    /**
     * Set the logging level only message with log_lvl smaller or equal then the set logging level will be logged.
     * The default level is 2, only message with log_lvl 2,1 and 0 are printed.
     * Note:
     * Logging level shall indicate how detailed the loggin output is. Higher means more detailled.
     * A logging level 0 shall only be used for fatal errors inside the Gateway and will be always logged.
     * @param log_lvl
     */
    virtual void set_log_lvl(uint8_t log_lvl)=0;

    /**
     * logs a single line with timestamp or whatever at the beginning.
     * @param msg to be logged
     */
    virtual void log(char *msg, uint8_t log_lvl)=0;

    /**
     * logs a single line with timestamp or whatever at the beginning.
     * @param msg to be logged
     */
    virtual void log(const char *msg, uint8_t log_lvl)=0;

    /**
     * logs a line can be appended with append_log
     * @param msg
     */
    virtual void start_log(char *msg, uint8_t log_lvl)=0;

    /**
     * logs a line can be appended with append_log
     * @param msg
     */
    virtual void start_log(const char *msg, uint8_t log_lvl)=0;


    /**
     * sets the log lvl for log message to be printed
     * @param msg
     */
    virtual void set_current_log_lvl(uint8_t log_lvl)=0;

    /**
     * append message to the last log message
     * @param msg to be appended
     */
    virtual void append_log(char *msg)=0;

    /**
     * append message to the last log message
     * @param msg to be appended
     */
    virtual void append_log(const char *msg)=0;

};

#endif //ESPARDUINOMQTTSNGATEWAY_LOGGERINTERFACE_H
