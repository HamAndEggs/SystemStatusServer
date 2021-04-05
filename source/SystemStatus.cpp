#include "SystemStatus.h"
#include "TinyTools.h"

#include <chrono>
#include <string>
#include <sstream>


SystemStatus::SystemStatus() :
    mKeepRunning(true)
{
    mGatherThread = std::thread([this]()
    {
        mJsonSystemStatus = "code booting";
        tinytools::MillisecondTicker tickEveryMinute(1000 * 60);

        std::map<int,tinytools::system::CPULoadTracking> trackingData;
        
        tinytools::system::GetCPULoad(trackingData,mCPULoads);
        mIpAddress = tinytools::network::GetLocalIP();
        mHostName = tinytools::network::GetHostName();
        RebuildJson();

        int n = 0;
        while( GetKeepRunning() )
        {
            bool rebuildJson = false;
            // This is where we do a lot of work.
            if( tickEveryMinute.Tick() )
            {
                if( tinytools::system::GetCPULoad(trackingData,mCPULoads) )
                {
                    rebuildJson = true;
                }
            }

            if( rebuildJson )
            {
                RebuildJson();
            }

            // Check again in a second. Not doing big wait here as I need to be able to quit in a timely fashion.
            // I guess there is a much better way to do this, I will look at some point.....
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
}

SystemStatus::~SystemStatus()
{
    mKeepRunning = false;
    if( mGatherThread.joinable() )
    {
        mGatherThread.join();
    }
}

struct JsonWriter
{
    std::stringstream ss;
    char comma = ' ';
    bool mHasData = false;

    JsonWriter()
    {
    }

    std::string ToString()
    {
        return ss.str();
    }

    void AddObject(const std::string& pKey,const std::string& pValue)
    {
        ss << comma << "\"" << pKey << "\":\"" << pValue << "\"";
        comma = ',';
        mHasData = true;
    }

    void ArrayAddObject(const std::string& pKey,const std::string& pValue)
    {
        ss << comma << "{\"" << pKey << "\":\"" << pValue << "\"}";
        comma = ',';
        mHasData = true;
    }

    void AddArray(const std::string& pKey,const std::string& pValue)
    {
        ss << comma << "\"" << pKey << "\":[" << pValue << "]";
        comma = ',';
        mHasData = true;
    }
};

void SystemStatus::RebuildJson()
{
    JsonWriter json;

    if( mIpAddress.size() > 0 )
    {
        json.AddObject("ip",mIpAddress);
    }

    if( mHostName.size() > 0 )
    {
        json.AddObject("name",mHostName);
    }

    if( mCPULoads.size() > 0 )
    {
        JsonWriter loads;
        for(const auto& core : mCPULoads)
        {
            std::string key = "cpu";
            if( core.first >= 0 )
            {
                key += std::to_string(core.first);
            }
            loads.ArrayAddObject(key,std::to_string(core.second));
        }
        json.AddArray("cpu_load",loads.ToString());
    }

    if( json.mHasData )
    {
        std::lock_guard<std::mutex> guard(mJsonSystemStatusMutex);
        mJsonSystemStatus = "{" + json.ToString() + "}";// Safely make a copy.
    }
}
