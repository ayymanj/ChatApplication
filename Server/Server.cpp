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
using namespace std;

map<string, SOCKET> clients;                    // username -> socket
map<string,vector<string>> groups;   // groupname -> usernames
mutex clients_mutex;

void UserList() {
    clients_mutex.lock();
    string userList = "USERS";
    map<string, SOCKET>::iterator client_mapper;
    for (client_mapper = clients.begin(); client_mapper != clients.end();++client_mapper) {
        string username = client_mapper->first;
        userList += username + ", ";
    }
    for (client_mapper = clients.begin(); client_mapper != clients.end(); ++client_mapper) {
        SOCKET clientSocket = client_mapper->second; 
        send(clientSocket, userList.c_str(), userList.length(), 0);
    }

    clients_mutex.unlock();
}

void handleClient(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    string username;
    // client sends the username
    int len = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (len <= 0) {
        closesocket(clientSocket);
        return;
    }
    buffer[len] = '\0';
    username = buffer;

    {
        lock_guard<mutex> lock(clients_mutex);
        clients[username] = clientSocket;
    }

    UserList();
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
            istringstream ss(msg);
            string cmd, targetUser, message;
            ss >> cmd >> targetUser;
            std::getline(ss, message);
            message = "[Individual (DM) from " + username + "]:" + message;

            lock_guard<mutex> lock(clients_mutex);
            if (clients.find(targetUser) != clients.end()) {
                send(clients[targetUser], message.c_str(), message.length(), 0);
            }

        }
        else if (msg.rfind("/group ", 0) == 0) {
            istringstream ss(msg);
            string cmd, groupName, usersStr;
            ss >> cmd >> groupName;
            getline(ss, usersStr);
            usersStr.erase(0, 1);

            istringstream userStream(usersStr);
            string user;
            vector<string> userList;

            while (getline(userStream, user, ',')) {
                userList.push_back(user);
            }

            lock_guard<mutex> lock(clients_mutex);
            groups[groupName]= userList;

        }
        else if (msg.rfind("/sendgroup ", 0) == 0) {
            istringstream ss(msg);
            string cmd, groupName, message;
            ss >> cmd >> groupName;
            getline(ss, message);
            message = "[Group " + groupName + " from " + username + "]:" + message;

            lock_guard<mutex> lock(clients_mutex);
            if (groups.find(groupName) != groups.end()) {
                vector<string>memberList = groups[groupName];
                for (int i = 0; i < memberList.size(); ++i) {
                    string memberName = memberList[i];
                    // Check if the member is currently connected (present in 'clients')
                    if (clients.find(memberName) != clients.end()) {
                        SOCKET memberSock = clients[memberName];
                        send(memberSock, message.c_str(), message.length(), 0);
                    }
                }

            }
        }
        else {
            string echo = "[Echo] " + msg;
            send(clientSocket, echo.c_str(), echo.length(), 0);
        }
    }

    {
        lock_guard<mutex> lock(clients_mutex);
        clients.erase(username);
    }

    printf("User %s disconnected.\n", username.c_str());
    UserList();
    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    SOCKET listenSocket, clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    int clientAddrLen = sizeof(clientAddress);

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

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(DEFAULT_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
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
        clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddress, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            printf("Accept failed.\n");
            continue;
        }

        thread(handleClient, clientSocket).detach();  
    }

    closesocket(listenSocket);
    WSACleanup();
    return 0;
}
