#include "SystemStatus.h"
#include "TinyTools.h"

#include <chrono>
#include <string>
#include <sstream>

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

    void AddObject(const std::string& pKey,int pValue)
    {
        ss << comma << "\"" << pKey << "\":" << pValue;
        comma = ',';
        mHasData = true;
    }   

    void AddObject(const std::string& pKey,int64_t pValue)
    {
        ss << comma << "\"" << pKey << "\":" << pValue;
        comma = ',';
        mHasData = true;
    }

    void AddObject(const std::string& pKey,size_t pValue)
    {
        ss << comma << "\"" << pKey << "\":" << pValue;
        comma = ',';
        mHasData = true;
    }

    void AddObject(const std::string& pKey,const std::string& pValue)
    {
        if( pValue.size() > 0 )
        {
            ss << comma << "\"" << pKey << "\":\"" << pValue << "\"";
            comma = ',';
            mHasData = true;
        }
    }

    void ArrayAddObject(const std::string& pKey,const std::string& pValue)
    {
        if( pValue.size() > 0 )
        {
            ss << comma << "{\"" << pKey << "\":\"" << pValue << "\"}";
            comma = ',';
            mHasData = true;
        }
    }

    void ArrayAddValue(const std::string& pValue)
    {
        if( pValue.size() > 0 )
        {
            ss << comma << "\"" << pValue << "\"";
            comma = ',';
            mHasData = true;
        }
    }

    void ArrayAddValue(size_t pValue)
    {
        ss << comma << pValue;
        comma = ',';
        mHasData = true;
    }

    void ArrayAddValue(int64_t pValue)
    {
        ss << comma << pValue;
        comma = ',';
        mHasData = true;
    }

    void ArrayAddValue(int pValue)
    {
        ss << comma << pValue;
        comma = ',';
        mHasData = true;
    }

    void AddArray(const std::string& pKey,const std::string& pValue)
    {
        if( pValue.size() > 0 )
        {
            ss << comma << "\"" << pKey << "\":[" << pValue << "]";
            comma = ',';
            mHasData = true;
        }
    }

};

SystemStatus::SystemStatus(int pUpdateInterval) 
{
    mGatherThread.Tick(pUpdateInterval,[this]()
    {
        const std::string ipAddress = tinytools::network::GetLocalIP();
        const std::string hostName = tinytools::network::GetHostName();
        const std::string uptime = tinytools::system::GetUptime();

        std::map<int,int> cpuLoads;
        tinytools::system::GetCPULoad(mCPULoadData,cpuLoads);

        size_t totalMemory,freeMemory,totalSwap,freeSwap;
        tinytools::system::GetMemoryUsage(totalMemory,freeMemory,totalSwap,freeSwap);
        
        JsonWriter json;
        
        json.AddObject("time",tinytools::system::SecondsSinceEpoch());// So caller can see when the data has been updated.
        json.AddObject("ip",ipAddress);
        json.AddObject("name",hostName);
        json.AddObject("up_time",uptime);

        json.AddObject("total_memory",totalMemory);
        json.AddObject("free_memory",freeMemory);
        json.AddObject("total_swap",totalSwap);
        json.AddObject("free_swap",freeSwap);

        if( cpuLoads.size() > 0 )
        {
            JsonWriter loads;
            for(const auto& core : cpuLoads)
            {
                loads.ArrayAddValue(core.second);
            }
            json.AddArray("cpu_load",loads.ToString());
        }

        // And over write old data safely and a quickly as we can.
        // Note how I only take the lock when I am ready to write data.
        {
            std::lock_guard<std::mutex> guard(mJsonSystemStatusMutex);
            mJsonSystemStatus = "{" + json.ToString() + "}";// Safely make a copy.
        }
    });
}

SystemStatus::~SystemStatus()
{
    mGatherThread.TellThreadToExitAndWait();
}
