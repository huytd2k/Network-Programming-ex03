#pragma once

#include <stdio.h>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <string>
#include <vector>
#include <process.h>
#include <string>
#include <iostream>
#include <limits>

#define BUFF_SIZE 10000
#define delimiter "/r/n"

#define LOGIN_FLAG "1"
#define POST_FLAG "2"
#define LOGOUT_FLAG "3"


#pragma comment(lib, "Ws2_32.lib")

// Utils
std::pair<int, char*> parseArgs(int argc, char* argv[]);
std::string extractFromBuffer(char* buffer);

// Winsocks operations
WSADATA initWinsock();
SOCKET initClientSocket();
sockaddr_in initServerAddr(int port, char* addr);
int sendStdStrMessage(SOCKET connSocket, std::string message);
void connectToServer(SOCKET client, sockaddr_in serverAddr);

void menu(SOCKET soc);
void printMenu();
void handleLogin(SOCKET soc);
void handlePost(SOCKET soc);
void login(std::string username, SOCKET soc);
std::string receiveStdString(SOCKET soc);
void handleLogout(SOCKET soc);
