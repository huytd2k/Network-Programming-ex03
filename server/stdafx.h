#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <string>
#include <vector>
#include <process.h>
#include <string>
#include <fstream>
#include <iostream>
#include <cassert>
#include <time.h>

#define server_addr "127.0.0.1"
#define BUFF_SIZE 2048
#define delimiter "/r/n"

#define LOGIN_FLAG "1"
#define POST_FLAG "2"
#define LOGOUT_FLAG "3"

#pragma comment(lib, "Ws2_32.lib")



// Prototypes
void process(std::string data, SOCKET soc, std::string addr, int port);
void loadAccounts();
void addAcount(std::string username, bool locked);
void handleLoginReq(std::string username, SOCKET soc, std::string clientIp, int clientPort);
void handlePostReq(std::string post, SOCKET soc, std::string addr, int port);
void handleLogoutReq(SOCKET soc, std::string addr, int port);
void logToFile(std::string clientIp, int clientPort, std::string data);
std::string makeLog(std::string flag, std::string data, std::string code);
std::string getFormatedTime();
// Main
unsigned __stdcall handleOneClient(void *param);

// Utils
int getPortNumber(int argc, char* argv[]);
std::string extractFromBuffer(char* buffer);

// Winsocks operations
WSADATA initWinsock();
SOCKET initListenSocket();
sockaddr_in initServerAddr(int port);
void bindListenSocAndStartListen(SOCKET listenSock, sockaddr_in serverAddr);
std::pair<SOCKET, std::string> acceptConnection(SOCKET listenSock);
int sendStdStrMessage(SOCKET connSocket, std::string message);


