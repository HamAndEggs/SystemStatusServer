#ifndef WEB_SERVER_H__
#define WEB_SERVER_H__

#include <functional>
#include <string>

class WebServer
{
public:
    WebServer(int pPort,const std::string& pAccessToken,const std::function<std::string()> pGetSystemStatusCB);
    ~WebServer();

    std::string GetSystemStatus()const
    {
        if( mGetSystemStatusCB != nullptr )
        {
            return mGetSystemStatusCB();
        }
        return "";
    }

    const std::string& GetAccessToken()const{return mAccessToken;}

private:
    const int mPort;
    const std::function<std::string()> mGetSystemStatusCB;
    const std::string mAccessToken;
    struct MHD_Daemon* theDaemon;

};

#endif //#ifndef WEB_SERVER_H__

