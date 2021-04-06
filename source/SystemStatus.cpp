#include "SystemStatus.h"
#include "TinyTools.h"

#include <chrono>
#include <string>
#include <sstream>

SystemStatus::SystemStatus(int pUpdateInterval) 
{
    tinytools::system::GetCPULoad(mCPULoadData,mCPULoads);

    mGatherThread.Tick(pUpdateInterval,[this]()
    {
        mIpAddress = tinytools::network::GetLocalIP();
        mHostName = tinytools::network::GetHostName();
        mUptime = tinytools::system::GetUptime();

        tinytools::system::GetCPULoad(mCPULoadData,mCPULoads);
        RebuildJson();
    });
}

SystemStatus::~SystemStatus()
{
    mGatherThread.TellThreadToExitAndWait();
}

/**
 * @brief Quick and dirty json writer.
 */
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

    if( mUptime.size() > 0 )
    {
        json.AddObject("up_time",mUptime);
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
