#include <stdint.h>
#include <periodic_task.hpp>
#pragma once

template <typename T> 
class SyncedProperty {
protected:
    T serverLastUpdatedValue;
    T serverCurrentValue;
    uint64_t serverMillsAtLastChange;
    T localLastUpdatedValue;
    T localCurrentValue;
    T localDebouncingValue;
    uint64_t localMillsAtLastChange;
    uint64_t localDebounceMills;
public:
    typedef struct {
        T value;
        uint64_t millsSinceUpdate;
    } value_t;
    SyncedProperty(T defaultValue, uint64_t localDebounceMills = 0): localDebounceMills(localDebounceMills) {
        serverLastUpdatedValue = defaultValue;
        serverCurrentValue = defaultValue;
        serverMillsAtLastChange = PeriodicTask::getCurrentTimeInMilliseconds();
        localLastUpdatedValue = defaultValue;
        localCurrentValue = defaultValue;
        localMillsAtLastChange = PeriodicTask::getCurrentTimeInMilliseconds();
    };
    void UpdateServer(T serverUpdate) {
        if (serverUpdate == serverCurrentValue) return;
        uint64_t currentTime = PeriodicTask::getCurrentTimeInMilliseconds();
        serverLastUpdatedValue = serverCurrentValue;
        serverCurrentValue = serverUpdate;
        serverMillsAtLastChange = currentTime;
    }
    void UpdateLocal(T localUpdate) {
        if (localCurrentValue == localUpdate) return;
        uint64_t currentTime = PeriodicTask::getCurrentTimeInMilliseconds();
        localLastUpdatedValue = localCurrentValue;
        localCurrentValue = localUpdate;
    }
    value_t GetLocal() {
        return {
            localCurrentValue,
            PeriodicTask::getCurrentTimeInMilliseconds() - localMillsAtLastChange
        };
    }
    value_t GetServer() {
        return {
            serverCurrentValue,
            PeriodicTask::getCurrentTimeInMilliseconds() - serverMillsAtLastChange
        };
    }
    // Get the one that has been updated most recently
    value_t GetValue() {
        value_t local = GetLocal();
        value_t server = GetServer();
        if (local.millsSinceUpdate < server.millsSinceUpdate) return local;
        return server;
    }
};