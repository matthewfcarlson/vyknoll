#pragma once

#include <periodic_task.hpp>

class TimeOut {
protected:
    uint32_t millsUntilTimeout;
    uint64_t startTime = 0;
    #ifdef PIO_UNIT_TESTING
    uint64_t fakeClockMillis = 0;
    #endif

public:
    TimeOut(uint32_t millsUntilTimeout): millsUntilTimeout(millsUntilTimeout){
        Start();
    };
    void Start() {
        startTime = PeriodicTask::getCurrentTimeInMilliseconds();
    }
    bool Check() {
        uint64_t currentTime = PeriodicTask::getCurrentTimeInMilliseconds();
        if (currentTime >= (this->startTime + millsUntilTimeout)) {
            return true;
        }
        return false;
    }

};