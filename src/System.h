#ifndef ESPARDUINOMQTTSNGATEWAY_SYSTEM_H
#define ESPARDUINOMQTTSNGATEWAY_SYSTEM_H

#include <stdint.h>
/**
 * Defines basic functionality provided by an underlying OS or must be implemented otherwise.
 */
class System {
public:
    /**
     * Sets the heartbeat value where the System perform regular checks.
     * Default value is a period of 30 000 ms
     * @param period
     */
    virtual void set_heartbeat(uint32_t period)=0;

    /**
     * Gets the heartbeat value where the System perform regular checks.
     * Default value is a period of 30 000 ms
     */
    virtual uint32_t get_heartbeat()=0;

    /**
     * Checks if the heartbeat is timed out.
     * @return true if the heartbeat value was reached, false otherwise.
     */
    virtual bool has_beaten()=0;

    /**
     * Get the elapsed time between two calls of this function.
     * @return the elapsed time between two calls
     */
    virtual uint32_t get_elapsed_time()=0;

    /**
     * Lets the execution sleep for some seconds.
     */
     virtual void sleep(uint32_t duration)=0;

    /**
     * Stop/exit the whole program or restart it if you want to.
     */
    virtual void exit()=0;
};

#endif //ESPARDUINOMQTTSNGATEWAY_SYSTEM_H
