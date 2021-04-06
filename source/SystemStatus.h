#ifndef SYSTEM_STATUS_H
#define SYSTEM_STATUS_H

#include "TinyTools.h"

#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>

class SystemStatus
{
public:
    SystemStatus(int pUpdateInterval);
    ~SystemStatus();

    /**
     * @brief Thread safe way to get the system status
     * Notice how I send back a copy!
     */
    std::string GetmJsonSystemStatus()const
    {
        std::lock_guard<std::mutex> guard(mJsonSystemStatusMutex);
        std::string json = mJsonSystemStatus;// Safely take a copy.
        return json; // Return as a copy.
    }

private:
    tinytools::threading::SleepableThread mGatherThread;
    std::map<int,tinytools::system::CPULoadTracking> mCPULoadData;

    // This is the data we collect.
    std::string mIpAddress;
    std::string mHostName;
    std::string mUptime;

    std::map<int,int> mCPULoads;

    // This is the built json data ready to do as a string when and if the web server asks for it.
    std::string mJsonSystemStatus;
    mutable std::mutex mJsonSystemStatusMutex;

    void RebuildJson();

};


#endif //#ifndef SYSTEM_STATUS_H