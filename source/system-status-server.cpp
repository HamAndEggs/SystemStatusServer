#include "WebServer.h"
#include "SystemStatus.h"

#include "TinyTools.h"

#include <signal.h>

#include <string>
#include <iostream>
#include <mutex>


// Will be run as a service, so be nice in release to keep it quite.
#ifdef DEBUG_BUILD
    #define DBG_MESSAGE(__MESSAGE__)    {std::cout << (__MESSAGE__) << std::endl;}
#else
    #define DBG_MESSAGE(__MESSAGE__)
#endif

static std::mutex WaitForQuit;


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

int main(int argc, char *argv[])
{
    int port = 1969;
	int interval = 10;
	std::string magicToken;

	tinytools::CommandLineOptions cmdLine("system-status-server [OPTION]...");

	cmdLine.AddArgument('p',"port","Sets the port to listen on, default is 1969. allowed range, as defined in RFC 6335, 1024 -> 49151",required_argument,[&port](const std::string& pOptionalArgument)
	{
		port = std::stoi(pOptionalArgument);
	});

	cmdLine.AddArgument('t',"token","If set then the request must contain a string token that must match re receive the data.",required_argument,[&magicToken](const std::string& pOptionalArgument)
	{
		magicToken = pOptionalArgument;
	});

	cmdLine.AddArgument('i',"update-interval","A number, in seconds, that the service should rebuild it's internal data on the state of the system. Minumum is 10 seconds.",required_argument,[&interval](const std::string& pOptionalArgument)
	{
		interval = std::stoi(pOptionalArgument);
	});

	if( cmdLine.Process(argc,argv) )
	{
		// Check out values.
		if( interval < 10 )
		{
			std::cerr << "Update interval is invalid, allowed minumum is 10 seconds, you passed in " << interval << "\n";
			return EXIT_FAILURE;
		}

		// Warn about a daft value
		if( interval > 60*60 )
		{
			std::clog << "Warning, interval is greater than an hour, this may not create any information that is of any use\n";
		}

		// validate the port used, ports 1024-49151 are the User Ports and are the ones to use for your own protocols, see https://www.rfc-editor.org/info/rfc6335 
		if( port < 1024 || port > 49151 )
		{
			std::cerr << "Port number is invalid, allowed range, as defined in RFC 6335 is 1024 -> 49151 Your value " << port << "\n";
			return EXIT_FAILURE;
		}
	}
	else
	{
		return 0;
	}

	// Start the thread that gathers the information at different times, collatesthe info and will pass onto the web server using dependency injection.
	SystemStatus theSystemStatus(interval);

	// Start the web server.
	WebServer theServer(port,magicToken,[&theSystemStatus]()
	{
		return theSystemStatus.GetmJsonSystemStatus();
	});

    WaitForQuit.lock();// Grab this now, will be release when you do ctrl + c or a signal from the OS.

	signal (SIGINT,CtrlHandler);

	DBG_MESSAGE("System status server running");
	    WaitForQuit.lock();//Should block till we're done.
	DBG_MESSAGE("System status server stopped");

   	return EXIT_SUCCESS;
}