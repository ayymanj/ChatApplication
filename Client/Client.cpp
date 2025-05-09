#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <thread>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")
#define SERVER_IP "127.0.0.1" 
using namespace std;

#define SERVER_PORT 50000
#define BUFFER_SIZE 1024

SOCKET clientSocket;

void receiveMessages() {
    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            //clearly if user sends something and nothing is received then somethhign went wr
            printf("Disconnected from server.\n"); 
            closesocket(clientSocket);
            WSACleanup();
            exit(0);
        }

        buffer[bytesReceived] = '\0';
        // conversion to string
        string msg(buffer); 

        if (msg.rfind("USERS", 0) == 0) {
            cout << "\n[Connected users]: " << msg.substr(5) << "\n> ";
        }
        else {
            cout << "\n" << msg << "\n> ";
        }
    }
}

int main() {
    WSADATA wsaData;
    struct sockaddr_in serverAddr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed to start.\n");
        return -1;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        printf("The sockets creation has failed.\n");
        WSACleanup();
        return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Connection has failed.\n");
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    }

    string username;
    cout << "Input your username: ";

    getline(cin, username);

    send(clientSocket, username.c_str(), username.size(), 0);

    thread receiver(receiveMessages);
    receiver.detach();

    string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        if (input.empty()) continue;

        send(clientSocket, input.c_str(), input.length(), 0);
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
