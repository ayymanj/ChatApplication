#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <thread>
#include <map>
#include <vector>
#include <sstream>
#include <iostream>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 50000
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

std::map<std::string, SOCKET> clients;                    // username -> socket
std::map<std::string, std::vector<std::string>> groups;   // groupname -> usernames
std::mutex clients_mutex;

void broadcastUserList() {
    std::lock_guard<std::mutex> lock(clients_mutex);
    std::string userList = "USERLIST ";
    for (const auto& pair : clients) {
        userList += pair.first + " ";
    }

    for (const auto& pair : clients) {
        send(pair.second, userList.c_str(), userList.size(), 0);
    }
}

void handleClient(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    std::string username;

    // Step 1: Receive username
    int len = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (len <= 0) {
        closesocket(clientSocket);
        return;
    }
    buffer[len] = '\0';
    username = buffer;

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients[username] = clientSocket;
    }

    broadcastUserList();
    printf("User %s connected.\n", username.c_str());

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int recv_len = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (recv_len <= 0) {
            break;
        }

        buffer[recv_len] = '\0';
        std::string msg(buffer);

        // Handle commands
        if (msg.rfind("/msg ", 0) == 0) {
            std::istringstream ss(msg);
            std::string cmd, targetUser, message;
            ss >> cmd >> targetUser;
            std::getline(ss, message);
            message = "[Private from " + username + "]:" + message;

            std::lock_guard<std::mutex> lock(clients_mutex);
            if (clients.find(targetUser) != clients.end()) {
                send(clients[targetUser], message.c_str(), message.length(), 0);
            }

        }
        else if (msg.rfind("/group ", 0) == 0) {
            std::istringstream ss(msg);
            std::string cmd, groupName, usersStr;
            ss >> cmd >> groupName;
            std::getline(ss, usersStr);
            usersStr.erase(0, 1); // remove leading space

            std::istringstream userStream(usersStr);
            std::string user;
            std::vector<std::string> userList;

            while (std::getline(userStream, user, ',')) {
                userList.push_back(user);
            }

            std::lock_guard<std::mutex> lock(clients_mutex);
            groups[groupName] = userList;

        }
        else if (msg.rfind("/sendgroup ", 0) == 0) {
            std::istringstream ss(msg);
            std::string cmd, groupName, message;
            ss >> cmd >> groupName;
            std::getline(ss, message);
            message = "[Group " + groupName + " from " + username + "]:" + message;

            std::lock_guard<std::mutex> lock(clients_mutex);
            if (groups.find(groupName) != groups.end()) {
                for (const auto& member : groups[groupName]) {
                    if (clients.find(member) != clients.end()) {
                        send(clients[member], message.c_str(), message.length(), 0);
                    }
                }
            }
        }
        else {
            std::string echo = "[Echo] " + msg;
            send(clientSocket, echo.c_str(), echo.length(), 0);
        }
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(username);
    }

    printf("User %s disconnected.\n", username.c_str());
    broadcastUserList();
    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    SOCKET listenSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return -1;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        printf("Socket creation failed.\n");
        WSACleanup();
        return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(DEFAULT_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed.\n");
        closesocket(listenSocket);
        WSACleanup();
        return -1;
    }

    if (listen(listenSocket, MAX_CLIENTS) == SOCKET_ERROR) {
        printf("Listen failed.\n");
        closesocket(listenSocket);
        WSACleanup();
        return -1;
    }

    printf("Server started on port %d...\n", DEFAULT_PORT);

    while (true) {
        clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            printf("Accept failed.\n");
            continue;
        }

        std::thread(handleClient, clientSocket).detach();  // Handle client in separate thread
    }

    closesocket(listenSocket);
    WSACleanup();
    return 0;
}
