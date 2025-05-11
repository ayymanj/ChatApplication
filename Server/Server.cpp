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
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 50000
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024


using namespace std;
const string SHARED_KEY = "Zhuhai123";
map<string, SOCKET> clients;                    // username -> socket
map<string, vector<string>> groups;   // groupname -> usernames
mutex clients_mutex;

string xorEncryptDecrypt(const string& input, const string& key) {
    string output = input;
    for (size_t i = 0; i < input.size(); ++i)
        output[i] ^= key[i % key.size()];
    return output;
}


void UserList() {
    string userList = "USERS : ";
    string groupList = "GROUPS : ";
    string encryptedList;
    vector<SOCKET> socketsToSend;

    {
        lock_guard<mutex> lock(clients_mutex);

        // Build user list
        for (const auto& client : clients) {
            userList += client.first + " ";
            socketsToSend.push_back(client.second); // store sockets separately
        }

        // Build group list
        for (const auto& group : groups) {
            groupList += group.first + " ";
        }

        // Combine and encrypt
        string combined = userList + "\n" + groupList;
        encryptedList = xorEncryptDecrypt(combined, SHARED_KEY);
    }

    // Send OUTSIDE the lock
    for (SOCKET clientSocket : socketsToSend) {
        if (clientSocket != INVALID_SOCKET) {
            int lengthToSend = static_cast<int>(encryptedList.length());
            send(clientSocket, encryptedList.c_str(), lengthToSend, 0);
            ;
        }
    }
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
    string encryptedUsername(buffer);

    username = xorEncryptDecrypt(encryptedUsername, SHARED_KEY);

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
        string encryptedMsg(buffer);
        string msg = xorEncryptDecrypt(encryptedMsg, SHARED_KEY);

        // Handle commands
        if (msg.rfind("/msg ", 0) == 0) {
            istringstream ss(msg);
            string cmd, targetUser, message;
            ss >> cmd >> targetUser;
            getline(ss, message);
            message = "[Individual (DM) from " + username + "]:" + message;

            lock_guard<mutex> lock(clients_mutex);
            if (clients.find(targetUser) != clients.end()) {
                string encryptedResponse = xorEncryptDecrypt(message, SHARED_KEY);
                int LengthToSend = static_cast<int>(encryptedResponse.length());
                send(clients[targetUser], encryptedResponse.c_str(), LengthToSend, 0);
            }

        }
        else if (msg.rfind("/newgroup ", 0) == 0) {
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
           
            
            groups[groupName] = userList;
            UserList();
            

        }
        else if (msg.rfind("/sendgroup ", 0) == 0) {
            istringstream ss(msg);
            string cmd, groupName, message;
            ss >> cmd >> groupName;
            getline(ss, message);
            message = "[Group " + groupName + " from " + username + "]:" + message;

            lock_guard<mutex> lock(clients_mutex);
            if (groups.find(groupName) != groups.end()) {
                vector<string> memberList = groups[groupName];

                // Checking if sender is a group member
                bool isMember = find(memberList.begin(), memberList.end(), username) != memberList.end();
                if (!isMember) {
                    string error = xorEncryptDecrypt("You are not a member of this group.", SHARED_KEY);
                    send(clientSocket, error.c_str(), error.length(), 0);
                    return;
                }

                for (const string& memberName : memberList) {
                    if (clients.find(memberName) != clients.end()) {
                        SOCKET memberSock = clients[memberName];
                        string encrypted = xorEncryptDecrypt(message, SHARED_KEY);
                        int LengthToSend = static_cast<int>(encrypted.length());
                        send(memberSock, encrypted.c_str(), LengthToSend, 0);
                    }
                }
            }

        }
        else {
            string echo = "[Echo] " + msg;
            string encryptedEcho = xorEncryptDecrypt(echo, SHARED_KEY);
            int LengthToSend = static_cast<int>(encryptedEcho.length());
            send(clientSocket, encryptedEcho.c_str(), LengthToSend, 0);

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
