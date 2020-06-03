#include <string>
#include <iostream>
#include <mutex>

#include <string.h>
#include <signal.h>

#include <microhttpd.h> //sudo apt install libmicrohttpd-dev


// Will be run as a service, so be nice in release to keep it quite.
#ifdef DEBUG_BUILD
    #define DBG_MESSAGE(__MESSAGE__)    {std::cout << (__MESSAGE__) << std::endl;}
#else
    #define DBG_MESSAGE(__MESSAGE__)
#endif

std::mutex WaitForQuit;
void static CtrlHandler(int SigNum)
{
	static int numTimesAskedToExit = 0;
	std::cout << std::endl << "Asked to quit, please wait" << std::endl;
	if( numTimesAskedToExit > 2 )
	{
		std::cout << "Asked to quit to many times, forcing exit in bad way" << std::endl;
		exit(1);
	}
    WaitForQuit.unlock();// Will allow main thread to wake up and contine.
}

static int AccessHandlerCallback(void * theServerObject, struct MHD_Connection * connection,const char * url, const char * method, const char * version,const char * upload_data, size_t * upload_data_size, void ** ptr)
{
    std::string responce;
    // Don't care about the URL, just if it's a GET request.
    // Abusing HTTP here a bit, maybe. If we get a GET request, we send back out stats. Else say no.
    if( method && strcmp(method,"GET") == 0 )
    {
        responce = "{\"system\":\"me\"}";
    }
    else
    {
        responce = "<html><head><title>system status server</title></head><body>Access denied</body></html>";
    }

	MHD_Response *response = MHD_create_response_from_buffer(responce.size(),(void*)responce.c_str(), MHD_RESPMEM_MUST_COPY);
	int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);
	return ret;


    //All done, tell the library to stop calling us.
	return MHD_NO;
}

int main(int argc, char *argv[])
{
    const int port = 1969;

    WaitForQuit.lock();// Grab this now, will be release when you do ctrl + c or a signal from the OS.

	signal (SIGINT,CtrlHandler);

   	struct MHD_Daemon* theDaemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, port,
			NULL, NULL,
			AccessHandlerCallback, NULL,
			MHD_OPTION_NOTIFY_COMPLETED,NULL,NULL,
			MHD_OPTION_NONCE_NC_SIZE,10,
			MHD_OPTION_END);

	if( theDaemon == NULL )
	{
		std::cout << "The HTTPD server failed to run." << std::endl;
		return false;
	}

	DBG_MESSAGE("System status server running");

    WaitForQuit.lock();//Should block till we're done.

	if( theDaemon )
	{
		MHD_stop_daemon(theDaemon);
	}

	DBG_MESSAGE("System status server stopped");


   	return EXIT_SUCCESS;
}