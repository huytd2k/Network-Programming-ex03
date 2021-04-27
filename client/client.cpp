#include "stdafx.h"


#pragma comment(lib, "Ws2_32.lib")

int logged = false;

int main(int argc, char* argv[]) {
	// Parse arguments
	auto parsedArgs = parseArgs(argc, argv);

	// Get server information from arguments
	auto server_port = parsedArgs.first;
	auto server_addr = parsedArgs.second;

	// Initialize winsock
	auto wsaData = initWinsock();

	// Initialize a SOCK struct for client
	auto client = initClientSocket();

	// Handle timeout
	int tv = 1000;
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv), sizeof(int));

	// Initialize a sockaddr_in struct for server address
	auto serverAddr = initServerAddr(server_port, server_addr);

	// Try to connect to server
	connectToServer(client, serverAddr);

	char buff[BUFF_SIZE], clientIP[INET_ADDRSTRLEN];
	memset(buff, 0, BUFF_SIZE);
	int ret, messageLen;

	menu(client);

	// Close socket and clean up winsock
	closesocket(client);
	WSACleanup();
	return 0;
}

/*
This function print menu and handle user's choice
INPUT:	- SOCKET soc of current connected socket
*/
void menu(SOCKET sock) {
	int choice = -1;
	while (choice != 4) {
		printMenu();
		int choice;
		std::cout << "Your choice ->";
		std::cin >> choice;

		switch (choice) {
		case 1:
			handleLogin(sock);
			break;
		case 2:
			handlePost(sock);
			break;
		case 3:
			handleLogout(sock);
			break;
		case 4:
			break;
		default:
			printf("Invalid choice ! \n");
			break;
		}
	}
}

/*
Helper function to print menu
*/
void printMenu() {
	printf("---------------------------\n");
	printf("ENTER A CHOICE\n");
	printf("1. LOGIN \n");
	printf("2. POST \n");
	printf("3. LOGOUT \n");
	printf("4. EXIT \n");
}

/*
Handle login choice from user
INPUT:	- SOCKET struct of connected Socket with server
*/
void handleLogin(SOCKET connSock) {
	// If user is logged, do not login
	if (logged == true) {
		printf("ERROR: You are logged!\n");
		return;
	}
	// Receive username from keyboard
	printf("Enter username :");
	std::string username;
	std::cin >> username;

	// Call login with that username
	login(username, connSock);
}

/*
Send login request to server with connected socket
- INPUt: - username of account
- SOCKET struct of connected Socket with server
*/
void login(std::string username, SOCKET connSock) {
	std::string data;

	// Append flag to data header
	data.append(LOGIN_FLAG);

	// Append username to data payload
	data.append(username);

	// Send data
	sendStdStrMessage(connSock, data);

	// Wait result from server
	auto result = receiveStdString(connSock);

	// If server sent unexpected result, print error and exit
	if (result.length() < 3) {
		printf("Some thing wrong happened! \n");
		return;
	}

	// Get status of response
	auto status = result.substr(0, 3);

	// If status is 200 OK
	if (status == "200") {
		logged = true;
		std::cout << "You has been loged in as " << username << "\n";
	}
	else if (status == "401") printf("This account has been logged in another session! \n");
	// If status is 403
	else if (status == "403") printf("This account has been locked! \n");
	// If status is 404
	else if (status == "404") printf("There's no account with this username \n");
}

/*
Handle Post choice of user
INPUT: - connected socket soc
*/
void handlePost(SOCKET soc) {
	// Check if user is logged
	if (!logged) {
		printf("ERROR: You are logged!\n");
		return;
	}

	// First add post flag
	std::string data = POST_FLAG;
	std::string payload;
	const int POST_MAX_SIZE = BUFF_SIZE - 4;
	char buffer[POST_MAX_SIZE];

	// Get post from keyboard
	std::cout << "Enter post below: " << "\n";
	std::cin.ignore();
	std::cin.getline(buffer, sizeof(buffer));


	// Append to data
	data.append(buffer);

	// Send data
	sendStdStrMessage(soc, data);

	// Wait result from server
	auto result = receiveStdString(soc);

	// If server sent unexpected result, print error and exit
	if (result.length() < 3) {
		printf("Some thing wrong happened! \n");
		return;
	}

	//  Check result status
	auto status = result.substr(0, 3);
	if (status == "201") printf("You post has been posted sucessfully!\n");
}

/*
Handle Logout choice of user
INPUT: - connected socket soc
*/
void handleLogout(SOCKET soc) {
	// If user are not logged, print error and return
	if (logged == false) {
		printf("You are not logged! \n");
		return;
	}

	// First add Logout flag
	std::string data;
	data.append(LOGOUT_FLAG);

	// Send request
	sendStdStrMessage(soc, data);

	// Wait for response
	auto result = receiveStdString(soc);

	// If server sent unexpected result, print error and exit
	if (result.length() < 3) {
		printf("Some thing wrong happened! \n");
		return;
	}

	//  Check result status
	auto status = result.substr(0, 3);
	if (status == "300") { printf("You post has been logged out!\n"); }
	if (status == "301") printf("Cannot logout!");
	logged = false;
}

/*
This function help buffering from socket and return message in std string
INPUT:	- SOCKET struct soc of connected socket
*/

std::string receiveStdString(SOCKET soc) {
	// Init buffer
	char buff[BUFF_SIZE];
	memset(buff, 0, BUFF_SIZE);
	auto ret = recv(soc, buff, BUFF_SIZE, 0);

	// If error happens
	if (ret == SOCKET_ERROR) {
		if (WSAGetLastError() == WSA_WAIT_TIMEOUT) printf("Time out! \n");
		else printf("Error %d: Cannot receive data! \n", WSAGetLastError());
	}

	// Server return message with length more than 0
	else if (strlen(buff) > 0) {
		buff[ret] = 0;
		return std::string(buff);

	}
	return "NOMSG";
}

/*
Parsing server port and server address from arguments
INPUT:	- argc: number of arguments
- argv: arguments vector
RETURN: - a pair struct with serverport in first element and server address in second element
*/
std::pair<int, char*> parseArgs(int argc, char* argv[]) {
	if (argc < 3) {
		printf("Usage: client.exe <server address> <port number> \n");
		exit(0);
	}

	// Get server information from arguments
	auto server_port = atoi(argv[2]);
	char* server_addr = argv[1];
	return std::make_pair(server_port, server_addr);
}

std::string extractFromBuffer(char* buffer) {
	auto delimiterPos = strstr(buffer, delimiter);
	if (delimiterPos == NULL) {
		return std::string(buffer);
	}
	*delimiterPos = 0;
	return std::string(buffer);
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
Init client socket and return it
If cannot create socket, print error and exit
*/
SOCKET initClientSocket() {
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) {
		printf("Error %d: Cannot create client socket!. \n", WSAGetLastError());
		exit(0);
	}
	return client;
}


/*
Init an address for server socket
INPUT:	- an int port of server port
- a char* of af server address
RETURN: a sockaddr_in struct of server address
*/
sockaddr_in initServerAddr(int port, char* addr) {
	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, addr, &serverAddr.sin_addr);
	return serverAddr;
}


/*
Try to connect to server, if fails, print error and terminate process
INPUT:	- a SOCKET struct of client socket
- a sockaddr_in struct of server address
*/
void connectToServer(SOCKET client, sockaddr_in serverAddr) {
	// Try to connect to server
	if (connect(client, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
		printf("Error %d: Cannot connect to server.", WSAGetLastError());
		exit(0);
	}

	printf("Connected to server! \n");
}

/*
Use function to send a message to server by std string with delimiter
INPUT:  - SOCKET struct of connected socket
- std::string message to send
*/
int sendStdStrMessage(SOCKET connSock, std::string message) {
	message.append(delimiter);
	return	send(connSock, message.c_str(), message.length(), 0);
};
