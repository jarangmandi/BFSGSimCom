#include <thread>
#include <chrono>
#include <sstream>

#include "config.h"
#include "RangeWrapper.h"

bool RangeWrapper::cRun = false;

RangeWrapper::RangeWrapper(struct TS3Functions ts3Functions, Config* cfg)
{
	this->ts3Functions = ts3Functions;
	this->cfg = cfg;
}

void RangeWrapper::start(void)
{
	cRun = true;

	t1 = new std::thread(&RangeWrapper::worker, RangeWrapper(ts3Functions, cfg));
}

void RangeWrapper::stop(void)
{
	cRun = false;

	if (t1 != NULL)
		t1->join();
}

void RangeWrapper::worker(void)
{
	uint64 serverConnectionHandlerID = ts3Functions.getCurrentServerConnectionHandlerID();

	anyID myId;
	uint64 myChannel;
	anyID* clients;

	unsigned int result;

	while (cRun)
	{
		if (cfg->getRangeEffects())
		{
			// Work out who I am, which channel I'm in, then get a list of clients in the channel;
			result = ts3Functions.getClientID(serverConnectionHandlerID, &myId);
			result = ts3Functions.getChannelOfClient(serverConnectionHandlerID, myId, &myChannel);
			result = ts3Functions.getChannelClientList(serverConnectionHandlerID, myChannel, &clients);

			// Iterate through the list of clients that we've recovered
			for (anyID* clientIterator = clients; *clientIterator != NULL; clientIterator++)
			{
				// Only want to look at other people
				if (*clientIterator != myId)
				{
					char* clientPosition;
					ts3Functions.getClientVariableAsString(serverConnectionHandlerID, *clientIterator, CLIENT_META_DATA, &clientPosition);

					ts3Functions.freeMemory(clientPosition);
				}
			}

			// Free the memory used up by the client list.
			ts3Functions.freeMemory(clients);
		}

		// Fetch data from the simulator every 10 seconds
		std::this_thread::sleep_for(std::chrono::milliseconds(10000));

	}
};



std::string RangeWrapper::toString()
{
	std::ostringstream ostr;

	return ostr.str();
}


RangeWrapper::~RangeWrapper()
{
}
