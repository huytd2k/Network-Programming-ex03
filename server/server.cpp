#include "stdafx.h"
CRITICAL_SECTION CriticalSection;


// Struct to save thread session
typedef struct ActReq {
	SOCKET connSoc;
	std::string address;
	int port;
};

// Account class 
class Account {
public:
	std::string username;
	bool locked;
	bool logged;
	SOCKET connSOCK;
	std::string addr;
	int port;

	// Default constructor
	Account() {
		addr = "";
		connSOCK = NULL;
		locked = NULL;
		username = "";
		logged = NULL;
		port = NULL;
	}

	Account(std::string init_username, bool init_locked) {
		username = init_username;
		locked = init_locked;
		logged = false;
		connSOCK = NULL;
		addr = "";
		port = NULL;
	}
	Account(std::string init_username, bool init_locked, SOCKET soc, std::string add) {
		username = init_username;
		locked = init_locked;
		logged = false;
		connSOCK = soc;
		addr = add;
		port = NULL;
	}
};

// Global variable accounts to save loaded account from file
std::vector<Account> accounts;

int main(int argc, char* argv[]) {
	loadAccounts();

	// Initialize the critical section one time only.
	if (!InitializeCriticalSectionAndSpinCount(&CriticalSection,
		0x00000400))
		return 0;
	// Get port number

	auto server_port = getPortNumber(argc, argv);

	// Initialize winsock
	auto winsockData = initWinsock();

	// Initialize listen socket
	auto listenSock = initListenSocket();

	// Initialize server address to accept connections
	auto serverAddr = initServerAddr(server_port);

	// Bind server address to listenSock and start listening
	bindListenSocAndStartListen(listenSock, serverAddr);

	// Log to stdout
	printf("Server started! \n");

	while (1) {
		// Accept connection from client
		// Init sockaddr_in struct for client information
		sockaddr_in clientAddr;
		char  clientIP[INET_ADDRSTRLEN];
		int clientAddrLen = sizeof(clientAddr);

		// Accept connections from clients
		auto connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);

		// While accept return SOCKET_ERROR, try accept again
		while (connSock == SOCKET_ERROR) connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);

		// Convert address string to standard format
		inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));

		// Convert port to host order
		auto clientPort = ntohs(clientAddr.sin_port);

		ActReq* req = new ActReq;
		req->address = std::string(clientIP);
		req->port = clientPort;
		req->connSoc = connSock;

		printf("Accept incoming connection from %s:%d \n", clientIP, clientPort);
		printf("Start a thread \n");
		_beginthreadex(0, 0, handleOneClient, (void*)req, 0, 0);
	}

	// Close listen socket
	closesocket(listenSock);
	// Cleanup winsock
	WSACleanup();
	return 0;
}

// Create one thread for one connection
unsigned __stdcall handleOneClient(void* lpParam) {

	// Casting param to SOCKET Type
	ActReq* req = (ActReq*)lpParam;

	// Get thread session info
	auto connSock = req->connSoc;
	auto ip = req->address;
	auto port = req->port;


	char buff[BUFF_SIZE + 1];
	memset(buff, 0, BUFF_SIZE + 1);

	std::string dataBuffer = "";

	while (1) {
		// Start receiving
		auto ret = recv(connSock, buff, BUFF_SIZE, 0);
		if (ret == SOCKET_ERROR) {
			printf("Error %d: Cannot receive data. \n", WSAGetLastError());
			break;
		}
		printf("Received %d bytes \n", ret);
		buff[ret] = 0;

		// If recv return SOCKET_ERROR, print error, break
		if (ret == 0) break;
		// If received message has length more than 0
		else if (strlen(buff) > 0) {
			// Append extracted data from buffer
			dataBuffer.append(extractFromBuffer(buff));
			// If data now has delimter character, process command
			if (dataBuffer.find(delimiter) != std::string::npos) {
				process(dataBuffer, connSock, ip, port);
				// Reset data
				dataBuffer = "";
			}
		}
	}

	// End of connection
	printf("Closed one connect \n");
	handleLogoutReq(connSock, ip, port);
	// Close socket used in this thread
	closesocket(connSock);
	return 0;
}
/*
This function take data and handle it based on data's header
INPUT:	- std::string data
*/
void process(std::string data, SOCKET soc, std::string addr, int port) {
	std::cout << data << std::endl;
	// Find the delimiter in the end of data
	auto endPos = data.find(delimiter);
	// Assume that always have the delimter
	assert(endPos != NULL);

	// Extract status and payload from data
	auto flag = data.substr(0, 1);
	auto payload = data.substr(1, endPos - 1);
	std::cout << "client address:" << addr << "\n";
	std::cout << "status:" << flag << "\n";
	std::cout << "payload:" << payload << "\n";

	if (flag == "1") {
		handleLoginReq(payload, soc, addr, port);
	}
	else if (flag == "2") {
		handlePostReq(payload, soc, addr, port);
	}
	else if (flag == "3") {
		handleLogoutReq(soc, addr, port);
	}
	else sendStdStrMessage(soc, "INVALID REQUEST");
}
/* 
	This function handle logout request from client
	INPUT: 	- a SOCKET struct soc of connected socket to client
			- addr : client address
			- port : client port
*/
void handleLogoutReq(SOCKET soc, std::string addr, int port) {
	std::string resultCode = "";
	// Find in accounts in memory
	for (auto& user : accounts) {
		std::cout << "addr: " << user.addr << " " << user.port << "\n";

		// If this user has right session info
		if (user.addr == addr && user.port == port) {

			// If user already logged
			if (user.logged == true) {
				resultCode = "300";
				auto message = resultCode + user.username;

				// Thread sync
				EnterCriticalSection(&CriticalSection);
				user.logged = false;
				LeaveCriticalSection(&CriticalSection);

				sendStdStrMessage(soc, resultCode);
			}
			else {
				resultCode = "301";
				sendStdStrMessage(soc, resultCode);
			}
		}
	}
	logToFile(addr, port, makeLog(LOGOUT_FLAG, "", resultCode));
};

/* 
	This function handle POST request from client
	INPUT: 	- a SOCKET struct soc of connected socket to client
			- addr : client address
			- port : client port
*/
void handlePostReq(std::string post, SOCKET soc, std::string addr, int port) {
	std::string resultCode = "201";
	std::cout << "post:" << post << "\n";

	// Send successful response
	sendStdStrMessage(soc, resultCode);

	// Log
	logToFile(addr, port, makeLog(POST_FLAG, post, resultCode));
}

/* 
	This function handle LOGIN request from client
	INPUT: 	- a SOCKET struct soc of connected socket to client
			- addr : client address
			- port : client port
*/
void handleLoginReq(std::string username, SOCKET soc, std::string clientIp, int clientPort) {
	int found = 0;
	std::string resultCode = "";

	// Find in accounts
	for (auto& user : accounts) {
		// If find coressponse username
		if (user.username == username) {
			// if account is locked
			if (user.locked == true) {
				resultCode = "403";
				sendStdStrMessage(soc, resultCode);
			}
			// if account is logged in other session
			else if (user.logged == true) {
				resultCode = "401";
				sendStdStrMessage(soc, resultCode);
			}
			// if login OK
			else {
				resultCode = "200";
				auto message = resultCode + user.username;

				EnterCriticalSection(&CriticalSection);
				user.connSOCK = soc;
				user.addr = clientIp;
				sendStdStrMessage(soc, message);
				user.logged = true;
				user.port = clientPort;
				LeaveCriticalSection(&CriticalSection);

			}
			found = 1;
		}
	}
	// If dont found any user
	if (!found) {
		resultCode = "404";
		sendStdStrMessage(soc, "404");
	}

	//Log
	logToFile(clientIp, clientPort, makeLog(LOGIN_FLAG, username, resultCode));
}

/*
	Load account from file
*/
void loadAccounts() {
	std::string line;
	std::ifstream accountFile;

	printf("open file \n");

	// Open file
	accountFile.open("account.txt");

	// Check if file opened successfully
	if (accountFile.is_open()) {
		// Get line by line
		while (std::getline(accountFile, line)) {
			std::cout << line << std::endl;

			// Find the space dividing username and status
			auto spacePos = line.find(" ");

			// If there's no space in this line, show error line and exit
			if (spacePos == std::string::npos) {
				std::cout << "Account file has error: " << line << std::endl;
				exit(0);
			}

			// Extract username and status from file
			auto username = line.substr(0, spacePos);
			auto locked = line.substr(spacePos + 1) == "1";

			// Add account to local memory
			addAcount(username, locked);
		}
	}
	//Close file
	accountFile.close();
}


/*
This function add an account object to global variable accounts
INPUT:	- std::string - username or account
- bool looked - status of account
*/
void addAcount(std::string username, bool locked) {
	Account acc(username, locked);
	accounts.push_back(acc);
}

void logToFile(std::string clientIp, int clientPort, std::string data) {
	char filename[] = "log_20183768.txt";
	std::fstream logFile;

	// Open log file
	logFile.open(filename, std::fstream::in | std::fstream::out | std::fstream::app);

	// If file does not exist, Create new file
	if (!logFile)
	{
		std::cout << "Cannot load log, file does not exist. Creating new file..";
		logFile.open(filename, std::fstream::in | std::fstream::out | std::fstream::trunc);
		logFile << "\n";
	}


	logFile << clientIp << ":" << clientPort << " " << getFormatedTime() << " " << data << "\n";
	logFile.close();
}

// Helper function to create log format
std::string makeLog(std::string flag, std::string data, std::string code) {
	std::string reqType = "";

	// Based on flag
	if (flag == LOGIN_FLAG) reqType = "USER";
	else if (flag == POST_FLAG) reqType = "POST";
	else if (flag == LOGOUT_FLAG) reqType = "QUIT";
	auto ret = " $ " + reqType + " " + data + " $ " + code;
	return ret;
}

// Helper function to get formated time
std::string getFormatedTime() {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, 80, "[%d/%m/%Y %H:%M:%S]", timeinfo);

	return std::string(buffer);
}

/*
Get port number from command-line arguments
If arguments are invalid, print program's usage
*/
int getPortNumber(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Usage: server.exe <port number> \n");
		exit(0);
	}
	return atoi(argv[1]);
}

/*
Check if buffer have delimiter char
INPUT: char* buffer of buffer
RETURN: std::string message extracted from buffer
*/
std::string extractFromBuffer(char* buffer) {
	auto buffString = std::string(buffer);
	auto delimiterPos = buffString.find(delimiter);
	if (delimiterPos == std::string::npos) {
		return buffString;
	}
	return buffString.substr(0, delimiterPos + strlen(delimiter));
}


/*
Init winsock and return winsock data,
If winsock version not supported, exit process
*/
WSADATA initWinsock() {
	WSADATA wsaData;
	WORD wVer = MAKEWORD(2, 2);
	if (WSAStartup(wVer, &wsaData)) {
		printf("Winsock 2.2 is not supported !.");
		exit(0);
	}
	return wsaData;
}

/*
Init listen socket and return it
If cannot create socket, print error and exit
*/
SOCKET initListenSocket() {
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket!. \n", WSAGetLastError());
		exit(0);
	}
	return listenSock;
}


/*
Init an address for server socket
INPUT: a port for address
RETURN: a sockaddr_in struct of server address
*/
sockaddr_in initServerAddr(int port) {
	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, server_addr, &serverAddr.sin_addr);
	return serverAddr;
}


/*
Try to bind server address with a socket,
If it fails, terminal process
*/
void bindListenSocAndStartListen(SOCKET listenSock, sockaddr_in serverAddr) {
	if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
		printf("Error %d: Cannot assiociate a local address with server! \n", WSAGetLastError());
		exit(0);
	}

	if (listen(listenSock, 10)) {
		printf("Error");
		exit(0);
	}
}

/*
Accept connections from client
INPUT: a struct SOCKET of listen socket
RETURN: a struct SOCKET of connection socket
*/
std::pair<SOCKET, std::string> acceptConnection(SOCKET listenSock) {
	// Init sockaddr_in struct for client information
	sockaddr_in clientAddr;
	char  clientIP[INET_ADDRSTRLEN];
	int clientAddrLen = sizeof(clientAddr);

	// Accept connections from clients
	auto connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);

	// While accept return SOCKET_ERROR, try accept again
	while (connSock == SOCKET_ERROR) connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);

	// Convert address string to standard format
	inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));

	// Convert port to host order
	auto clientPort = ntohs(clientAddr.sin_port);
	printf("Accept incoming connection from %s:%d \n", clientIP, clientPort);
	return std::make_pair(connSock, std::string(clientIP));
}


/*
Sent an cpp std string through connection socket with flag set to 0:
INPUT:  - a SOCKET struct of connection socket
- a c++ string as message
RETURN: - winsock send result
*/
int sendStdStrMessage(SOCKET connSock, std::string message) {
	std::cout << "REV: " << message << "/n";
	return send(connSock, message.c_str(), message.length(), 0);
};

