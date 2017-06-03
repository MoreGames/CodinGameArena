#ifndef NETWORKING_H
#define NETWORKING_H

//#define DEBUG

#include <iostream>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <mutex>

#ifdef _WIN32
    #include <windows.h>
    #include <tchar.h>
    #include <stdio.h>
    #include <strsafe.h>
#else
    #include <signal.h>
    #include <time.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <sys/select.h>
#ifdef __linux__
    #include <sys/prctl.h>
#endif
    #include <unistd.h>
#endif

using namespace std;
using namespace std::chrono;

class Networking {
private:
#ifdef _WIN32
	struct IOHandles {
		HANDLE write, read;
	};
	struct ParrentChildConnection {
		IOHandles parent;
		IOHandles child;
	};
	std::vector<ParrentChildConnection> connections;
	std::vector<HANDLE> processes;
#else
	struct UniConnection {
		int read, write;
	};
	std::vector< UniConnection > connections;
	std::vector<int> processes;
#endif

	void sendString(size_t id, const string& sendString);
	string getString(size_t id, int timeLimit);

	bool isProcessDead(size_t id);
	string GetLastErrorAsString();

public:
	vector<string> botLogs;

    void startAndConnectBot(std::string command);
    long long handleInitNetworking(size_t id, int timeLimit, string initInput, string firstTurnInput, string* output);
    long long handleTurnNetworking(size_t id, size_t turn, int timeLimit, string input, string* output);
    void killBot(size_t id);
    size_t numberOfBots();
};

#endif
