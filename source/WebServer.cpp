#include "WebServer.h"

#include <microhttpd.h> //sudo apt install libmicrohttpd-dev
#include <string.h>

#include <map>
#include <stdexcept>
#include <iostream>

static MHD_Result EnumHeaders(void *cls, enum MHD_ValueKind kind,const char *key, const char *value)
{
	std::map<std::string,std::string>* headers = (std::map<std::string,std::string>*)cls;
	// at the moment I am only intrested in headers, I may add support for GET arguments.
	if( kind == MHD_HEADER_KIND )
	{
		const std::string theKey = key;
		const std::string theValue = value;
		headers->emplace(theKey,theValue);
	}
	return MHD_YES;
}

static MHD_Result AccessHandlerCallback(void * theServerObject, struct MHD_Connection * connection,const char * url, const char * method, const char * version,const char * upload_data, size_t * upload_data_size, void ** ptr)
{
    std::string responce;

	const WebServer* webServer = (const WebServer*)theServerObject;
	if( webServer == nullptr )
	{
		throw std::runtime_error("AccessHandlerCallback passed NULL web server object, I can't do anything without this!");
	}

	// Read the headers, we're looking to see if they supplied the option access token.
	std::map<std::string,std::string> headers;
	MHD_get_connection_values(connection,MHD_HEADER_KIND,EnumHeaders,&headers);

    // Don't care about the URL, just if it's a GET request.
    // Abusing HTTP here a bit, maybe. If we get a GET request, we send back out stats. Else say no.
    if( method && strcmp(method,"GET") == 0 )
    {
        const std::string& accessToken = webServer->GetAccessToken();
		// Are we looking for an access token?
		if( accessToken.size() > 0 )
		{
			// Did we get the access token?
			const auto& recivedToken = headers.find("access_token");
			if( recivedToken == headers.end() )
			{
				std::cerr << "Expected token, did not get one, aborting\n";
				return MHD_NO;
			}
			else if( recivedToken->second != accessToken )// Is it correct?
			{
				std::cerr << "Invalid token passed, aborting\n";
				return MHD_NO;
			}
            // all good, continue.
		}
        responce = webServer->GetSystemStatus();
    }
    else
    {
        return MHD_NO;
    }

	MHD_Response *response = MHD_create_response_from_buffer(responce.size(),(void*)responce.c_str(), MHD_RESPMEM_MUST_COPY);
	MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);
	return ret;
}


WebServer::WebServer(int pPort,const std::string& pAccessToken,std::function<std::string()> pGetSystemStatusCB) :
    mPort(pPort),
    mGetSystemStatusCB(pGetSystemStatusCB),
    mAccessToken(pAccessToken),
    theDaemon(nullptr)
{
   	theDaemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, mPort,
			NULL, NULL,
			AccessHandlerCallback, this,
			MHD_OPTION_NOTIFY_COMPLETED,NULL,NULL,
			MHD_OPTION_NONCE_NC_SIZE,10,
			MHD_OPTION_END);

	if( theDaemon == NULL )
	{
		throw std::runtime_error("The HTTPD server failed to run");
	}
}

WebServer::~WebServer()
{
	if( theDaemon )
	{
		MHD_stop_daemon(theDaemon);
	}
}
