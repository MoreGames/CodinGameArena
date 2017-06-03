#include "Networking.hpp"

void Networking::sendString(size_t id, const string& sendString) {
#ifdef _WIN32
	IOHandles connection = connections[id].parent;
	DWORD charsWritten;
	BOOL success;
	success = WriteFile(connection.write, sendString.c_str(), sendString.length(), &charsWritten, NULL);
	if (!success || charsWritten == 0) {
		string errorMsg("Problem writing to pipe\n");
		errorMsg += "WriteFile failed: " + GetLastErrorAsString() + "\n";
		cerr << errorMsg;
		throw 1;
	}
#else
	UniConnection connection = connections[id];
	ssize_t charsWritten = write(connection.write, sendString.c_str(), sendString.length());
	if (charsWritten < sendString.length()) {
		cerr << "Problem writing to pipe\n";
		throw 1;
	}
#endif
}

string Networking::getString(size_t id, int timeLimit) {
	string newString;
	long long timeoutMillisRemaining = timeLimit;
	high_resolution_clock::time_point tp = high_resolution_clock::now();

#ifdef _WIN32
	IOHandles connection = connections[id].parent;

	DWORD charsRead;
	BOOL success;
	char buffer;

	//Keep reading char by char until a newline
	while (true) {
		timeoutMillisRemaining = timeLimit - duration_cast<milliseconds>(high_resolution_clock::now() - tp).count();
		if (timeoutMillisRemaining < 0) {
			//cout << timeoutMillisRemaining << endl;
			throw newString;
		}
		// check to see that there are bytes in the pipe before reading
		// throw error if no bytes in alloted time
		// check for bytes before sampling clock, because reduces latency (vast majority the pipe is alread full)
		DWORD bytesAvailable = 0;
		BOOL isOK = PeekNamedPipe(connection.read, NULL, 0, NULL, &bytesAvailable, NULL);
		if (!isOK) {
			string errorMsg("");
			errorMsg += "PeekNamedPipe failed: " + GetLastErrorAsString() + "\n";
			cerr << errorMsg;
		}
		if (bytesAvailable < 1) {
			high_resolution_clock::time_point initialTime = high_resolution_clock::now();
			while (bytesAvailable < 1) {
				long long temp = duration_cast<milliseconds>(high_resolution_clock::now() - initialTime).count();
				if (temp > timeoutMillisRemaining) {
					//cout << " 2 " << temp << " " << timeoutMillisRemaining << endl;
					throw newString;
				}
				BOOL isOK = PeekNamedPipe(connection.read, NULL, 0, NULL, &bytesAvailable, NULL);
				if (!isOK) {
					string errorMsg("");
					errorMsg += "PeekNamedPipe failed: " + GetLastErrorAsString() + "\n";
					cerr << errorMsg;
				}
			}
		}

		success = ReadFile(connection.read, &buffer, 1, &charsRead, NULL);
		if (!success || charsRead < 1) {
			string errorMsg = "Bot #" + to_string(id) + " timed out or errored (Windows)\n";
			errorMsg += "ReadFile failed: " + GetLastErrorAsString() + "\n";
			cerr << errorMsg;
			throw newString;
		}
		if (buffer == '\n')	break;
		else newString += buffer;
	}
#else
	UniConnection connection = connections[id];

	fd_set set;
	FD_ZERO(&set); /* clear the set */
	FD_SET(connection.read, &set); /* add our file descriptor to the set */
	char buffer;

	//Keep reading char by char until a newline
	while (true) {

		//Check if there are bytes in the pipe
		timeoutMillisRemaining = timeoutMillis - duration_cast<milliseconds>(high_resolution_clock::now() - tp).count();
		if (timeoutMillisRemaining < 0) throw newString;
		struct timeval timeout;
		timeout.tv_sec = timeoutMillisRemaining / 1000.0;
		timeout.tv_usec = (timeoutMillisRemaining % 1000) * 1000;
		int selectionResult = select(connection.read + 1, &set, NULL, NULL, &timeout);

		if (selectionResult > 0) {
			read(connection.read, &buffer, 1);

			if (buffer == '\n') break;
			else newString += buffer;
		} else {
			string errorMessage = "Bot #" + to_string(id) + " timeout or error (Unix) " + to_string(selectionResult) + '\n';
			lock_guard<mutex> guard(coutMutex);
			cerr << errorMessage;
			throw newString;
		}
	}
#endif
	//Python turns \n into \r\n
	if (newString.back() == '\r') newString.pop_back();

#ifdef DEBUG
	string temp("Bot #" + to_string(id) + ":");
	temp += newString+"\n";
	cout << temp;
#endif

	return newString;
}

void Networking::startAndConnectBot(string command) {
#ifdef _WIN32
	//command = "/C " + command;

	IOHandles parentConnection, childConnection;

	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	//Child stdout pipe
	if (!CreatePipe(&parentConnection.read, &childConnection.write, &saAttr, 0)) {
		cerr << "Could not create pipe\n";
		throw 1;
	}
	if (!SetHandleInformation(parentConnection.read, HANDLE_FLAG_INHERIT, 0)) throw 1;

	//Child stdin pipe
	if (!CreatePipe(&childConnection.read, &parentConnection.write, &saAttr, 0)) {
		cerr << "Could not create pipe\n";
		throw 1;
	}
	if (!SetHandleInformation(parentConnection.write, HANDLE_FLAG_INHERIT, 0)) throw 1;

	//MAKE SURE THIS MEMORY IS ERASED
	PROCESS_INFORMATION piProcInfo;
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	STARTUPINFO siStartInfo;
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = INVALID_HANDLE_VALUE;
	siStartInfo.hStdOutput = childConnection.write;
	siStartInfo.hStdInput = childConnection.read;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	BOOL success = CreateProcess(
		LPSTR(command.c_str()),     //command line
		NULL,
		NULL,          //process security attributes
		NULL,          //primary thread security attributes
		TRUE,          //handles are inherited
		0,             //creation flags
		NULL,          //use parent's environment
		NULL,          //use parent's current directory
		&siStartInfo,  //STARTUPINFO pointer
		&piProcInfo
	);  //receives PROCESS_INFORMATION
	if (!success) {
		string errorMsg("CreateProcess failed: ");
		errorMsg += GetLastErrorAsString() + "\n";
		cerr << errorMsg;
		throw 1;
	} else {
		CloseHandle(piProcInfo.hThread);

		processes.push_back(piProcInfo.hProcess);
		connections.push_back({ parentConnection, childConnection });
	}
#else
	pid_t pid;
	int writePipe[2];
	int readPipe[2];

	if (pipe(writePipe)) {
		if (!quiet_output) cerr << "Error creating pipe\n";
		throw 1;
	}
	if (pipe(readPipe)) {
		if (!quiet_output) cerr << "Error creating pipe\n";
		throw 1;
	}

	pid_t ppid_before_fork = getpid();

	//Fork a child process
	pid = fork();
	if (pid == 0) { //This is the child
		setpgid(getpid(), getpid());

#ifdef __linux__
		// install a parent death signal
		// http://stackoverflow.com/a/36945270
		int r = prctl(PR_SET_PDEATHSIG, SIGTERM);
		if (r == -1)
		{
			cerr << "Error installing parent death signal\n";
			throw 1;
		}
		if (getppid() != ppid_before_fork)
			exit(1);
#endif

		dup2(writePipe[0], STDIN_FILENO);

		dup2(readPipe[1], STDOUT_FILENO);
		dup2(readPipe[1], STDERR_FILENO);

		execl("/bin/sh", "sh", "-c", command.c_str(), (char*)NULL);

		//Nothing past the execl should be run

		exit(1);
	} else if (pid < 0) {
		cerr << "Fork failed\n";
		throw 1;
	}

	UniConnection connection;
	connection.read = readPipe[0];
	connection.write = writePipe[1];

	connections.push_back(connection);
	processes.push_back(pid);

#endif

	//cout << "Connected bot: " << command << endl;
	botLogs.push_back(command+"\n");
}

long long Networking::handleInitNetworking(size_t id, int timeLimit, string initInput, string firstTurnInput, string* output) {
	try {
		sendString(id, initInput);
		sendString(id, firstTurnInput);
		botLogs[id] += " --- Init ---\n";

		high_resolution_clock::time_point initialTime = high_resolution_clock::now();
		(*output) = getString(id, timeLimit);
		long long millisTaken = duration_cast<milliseconds>(high_resolution_clock::now() - initialTime).count();
		botLogs[id] += *output + "\n --- Bot used " + to_string(millisTaken) + " milliseconds ---";
		return millisTaken;
	}
	catch (string s) {
		if (s.empty()) botLogs[id] += "\nERRORED!\nNo response received.";
		else botLogs[id] += "\nERRORED!\nResponse received (if any):\n" + s;
		botLogs[id] = "\nBot #" + to_string(id) + "; timed out during Init";
	}
	catch (...) {
		if ((*output).empty()) botLogs[id] += "\nERRORED!\nNo response received.";
		else botLogs[id] += "\nERRORED!\nResponse received (if any):\n" + *output;
		botLogs[id] = "\nBot #" + to_string(id) + "; timed out during Init";
	}
	return -1;
}

long long Networking::handleTurnNetworking(size_t id, size_t turn, int timeLimit, string input, string* output) {
	try {
		if (isProcessDead(id)) return -2;

		sendString(id, input);

		botLogs[id] += "\n-----------------------------------------------------------------------------\n --- turn #" + to_string(turn) + " ---\n";

		high_resolution_clock::time_point initialTime = high_resolution_clock::now();
		*output = getString(id, timeLimit);
		long long millisTaken = duration_cast<milliseconds>(high_resolution_clock::now() - initialTime).count();

		botLogs[id] += *output + "\n --- Bot used " + to_string(millisTaken) + " milliseconds ---";

		return millisTaken;
	}
	catch (string s) {
		if (s.empty()) botLogs[id] += "\nERRORED!\nNo response received.";
		else botLogs[id] += "\nERRORED!\nResponse received (if any):\n" + s;
		botLogs[id] = "\nBot #" + to_string(id) + "; timed out during turn " + to_string(turn);
		cerr << "\nERRORED!\nResponse received (if any):\n" + s << "\nBot #" + to_string(id) + "; timed out during turn " + to_string(turn) << endl;
	}
	catch (...) {
		if ((*output).empty()) botLogs[id] += "\nERRORED!\nNo response received.";
		else botLogs[id] += "\nERRORED!\nResponse received (if any):\n" + *output;
		botLogs[id] = "\nBot #" + to_string(id) + "; timed out during turn " + to_string(turn);
		cerr << "\nERRORED!\nResponse received (if any):\n" + *output << endl;
	}
	return -1;
}

void Networking::killBot(size_t id) {
	if (isProcessDead(id)) return;

	string newString;
	const int PER_CHAR_WAIT = 10; //millis
	const int MAX_READ_TIME = 100; //millis

#ifdef _WIN32
	// try to read entire contents of pipe.
	IOHandles connection = connections[id].parent;
	DWORD charsRead;
	BOOL success;
	char buffer;
	high_resolution_clock::time_point tp = high_resolution_clock::now();
	while (duration_cast<milliseconds>(high_resolution_clock::now() - tp).count() < MAX_READ_TIME) {
		DWORD bytesAvailable = 0;
		PeekNamedPipe(connection.read, NULL, 0, NULL, &bytesAvailable, NULL);
		if (bytesAvailable < 1) {
			high_resolution_clock::time_point initialTime = high_resolution_clock::now();
			while (bytesAvailable < 1) {
				if (duration_cast<milliseconds>(high_resolution_clock::now() - initialTime).count() > PER_CHAR_WAIT) break;
				PeekNamedPipe(connection.read, NULL, 0, NULL, &bytesAvailable, NULL);
			}
			if (bytesAvailable < 1) break; // took too long to get a character; breaking.
		}

		success = ReadFile(connection.read, &buffer, 1, &charsRead, NULL);
		if (!success || charsRead < 1) {
			string errorMsg = "Bot #" + to_string(id) + " timed out or errored (Windows)\n";
			errorMsg += "ReadFile failed: " + GetLastErrorAsString() + "\n";
			cerr << errorMsg;
			break;
		}
		newString += buffer;
	}

	HANDLE process = processes[id];

	success = TerminateProcess(process, 0);
	if (!success) {
		string errorMsg("TerminateProcess failed: ");
		errorMsg += GetLastErrorAsString() + "\n";
		cerr << errorMsg;
	}

	CloseHandle(process);
	CloseHandle(connections[id].parent.read);
	CloseHandle(connections[id].parent.write);
	CloseHandle(connections[id].child.read);
	CloseHandle(connections[id].child.write);

	processes[id] = NULL;
	connections[id].parent.read = NULL;
	connections[id].parent.write = NULL;
	connections[id].child.read = NULL;
	connections[id].child.write = NULL;

	//string deadMessage = "Player " + to_string(id) + " is dead\n";
	//cout << deadMessage;

#else
	// try to read entire contents of pipe.
	UniConnection connection = connections[id];
	fd_set set;
	FD_ZERO(&set); // clear the set
	FD_SET(connection.read, &set); // add our file descriptor to the set
	char buffer;
	high_resolution_clock::time_point tp = high_resolution_clock::now();
	while (duration_cast<milliseconds>(high_resolution_clock::now() - tp).count() < MAX_READ_TIME) {
		struct timeval timeout;
		timeout.tv_sec = PER_CHAR_WAIT / 1000;
		timeout.tv_usec = (PER_CHAR_WAIT % 1000) * 1000;
		int selectionResult = select(connection.read + 1, &set, NULL, NULL, &timeout);
		if (selectionResult > 0) {
			read(connection.read, &buffer, 1);
			newString += buffer;
		} else break;
	}

	kill(-processes[id], SIGKILL);

	processes[id] = -1;
	connections[id].read = -1;
	connections[id].write = -1;
#endif

	if (!newString.empty()) botLogs[id] += "\n --- Bot was killed. Below is the rest of its output (if any): ---\n" + newString + "\n --- End bot output ---";
}

bool Networking::isProcessDead(size_t id) {
#ifdef _WIN32
	return processes[id] == NULL;
#else
	return processes[id] == -1;
#endif
}

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
string Networking::GetLastErrorAsString() {
	//Get the error message, if any.
	DWORD errorMessageID = GetLastError();
	if (errorMessageID == 0) {
		return string(); //No error message has been recorded
	}

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}

size_t Networking::numberOfBots() {
	return connections.size();
}
