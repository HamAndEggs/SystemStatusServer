#ifndef SYSTEM_STATUS_H
#define SYSTEM_STATUS_H

#include <map>
#include <thread>
#include <mutex>

class SystemStatus
{
public:
    SystemStatus();
    ~SystemStatus();

    bool GetKeepRunning()const{return mKeepRunning;}

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
    std::thread mGatherThread;  //<! The worker thread that is gathering all the data for the web server to send out.
    bool mKeepRunning; //<! A boolean that will be used to signal the worker thread that it should exit.

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