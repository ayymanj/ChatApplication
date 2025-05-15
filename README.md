# 💬 Encrypted Chat Application

A secure real-time chat application featuring encrypted messaging over TCP/IP, built with a C++ server and Python GUI client. This project implements both individual and group messaging capabilities with a focus on secure communication.

## 🌟 Features

- **Secure Communication**
  - Encrypted message transmission
  - Private messaging support
  - Group chat functionality

- **User-Friendly Interface**
  - Modern PyQt5-based GUI
  - Real-time message updates
  - Easy-to-use group management
  - Username customization

- **Robust Architecture**
  - C++ server for efficient performance
  - Python client for flexible UI
  - TCP/IP socket communication
  - Multi-user support

## 🛠️ Tech Stack

- **Server Side**
  - C++ 
  - WinSock2 for network operations
  - Multi-threading for concurrent connections

- **Client Side**
  - Python
  - PyQt5 for GUI
  - Socket programming for network communication

- **Communication**
  - TCP/IP protocol
  - Custom encryption implementation
  - Real-time message handling

## 📋 Prerequisites

- Windows OS (for server)
- Python 3.x
- PyQt5 library
- C++ compiler (for server compilation)
- Visual Studio (for building server)

## 🚀 Getting Started

1. **Server Setup**
   ```bash
   # Navigate to x64/Debug directory
   # Run the server executable
   Server.exe
   ```

2. **Client Setup**
   ```bash
   # Navigate to Client-GUI directory
   # Run the Python client
   python client.py
   ```

3. **Usage**
   - Enter your username when prompted
   - Start sending messages
   - Create groups using the group chat feature
   - Send private messages to other users

## 💡 Features in Detail

### Individual Messaging
- Direct messaging between users
- Encrypted message transmission
- Real-time message delivery

### Group Chat
- Create custom groups
- Add multiple users to groups
- Send messages to all group members
- Group management features

### Security
- Message encryption
- Secure socket communication
- User authentication

## 🏗️ Project Structure
ChatApplication/
├── Server/ # C++ server implementation
├── Client-GUI/ # Python GUI client
│ └── client.py # Main client application
├── Client/ # Console client (prototype)
└── INSTRUCTIONS.txt # Setup instructions 

## 🔧 Configuration

- Server runs on localhost (127.0.0.1)
- Default port: 50000
- Supports multiple simultaneous connections
- Configurable encryption keys

## 📝 Usage Instructions

1. Start the server application first
2. Launch the client GUI
3. Enter your username
4. Start chatting:
   - Send individual messages
   - Create groups
   - Send group messages
   - View active users

## 🤝 Contributing

Feel free to fork this project and submit pull requests for any improvements.

## 📄 License

This project is available for open use.

## ⚠️ Note

This is a demonstration project showcasing encrypted chat functionality. While it implements basic encryption, additional security measures should be implemented for production use.
