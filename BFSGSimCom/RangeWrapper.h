#pragma once

#include <windows.h>

#include <thread>

#include "config.h"
#include "ts3_functions.h"

class RangeWrapper
{
private:
	static bool cRun;

	void worker(void);

	struct TS3Functions ts3Functions;
	Config* cfg;

	std::thread* t1 = NULL;

public:

	RangeWrapper(struct TS3Functions, Config*);
	~RangeWrapper();

	void start(void);
	void stop(void);

	static std::string toString();
};


