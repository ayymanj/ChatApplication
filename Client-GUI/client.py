import socket
import threading
from PyQt5.QtWidgets import QApplication, QWidget, QTextEdit, QLineEdit, QPushButton, QVBoxLayout, QInputDialog
import sys
from PyQt5.QtWidgets import QHBoxLayout  

SHARED_KEY = "Zhuhai123"  

class ChatClient(QWidget):
    def xor_encrypt_to_hex(self,text: str, key: str) -> str:
        return ''.join(f"{ord(c) ^ ord(key[i % len(key)]):02x}" for i, c in enumerate(text))
    def xor_decrypt_from_hex(self,hex_str: str, key: str) -> str:
        output = ""
        for i in range(0, len(hex_str), 2):
            byte = int(hex_str[i:i+2], 16)
            output += chr(byte ^ ord(key[(i // 2) % len(key)]))
        return output

    

   
   


    def __init__(self):
        super().__init__()
        self.username = ""
        self.setWindowTitle("C++ Server Chat")
        self.resize(450, 550)

        # Username label at the top
        self.username_display = QTextEdit()
        self.username_display.setReadOnly(True)
        self.username_display.setFixedHeight(40)

        self.chat_display = QTextEdit()
        self.chat_display.setReadOnly(True)
        self.chat_display.setStyleSheet("background-color: #ffffff;")

        self.message_input = QLineEdit()
        self.message_input.setPlaceholderText("Type your message here...")
        self.send_button = QPushButton("Send")

                # --- Menu Buttons ---
        self.menu_msg = QPushButton("Message User")
        self.menu_group = QPushButton("Make Group")
        self.menu_send_group = QPushButton("Send Group Message")

        menu_layout = QHBoxLayout()
        menu_layout.addWidget(self.menu_msg)
        menu_layout.addWidget(self.menu_group)
        menu_layout.addWidget(self.menu_send_group)

        layout = QVBoxLayout()
        layout.setSpacing(10)
        layout.addWidget(self.username_display)
        layout.addWidget(self.chat_display)
        layout.addLayout(menu_layout)
        layout.addWidget(self.message_input)
        layout.addWidget(self.send_button)
        self.setLayout(layout)

        # --- Menu button behavior ---
        self.menu_msg.clicked.connect(lambda: self.message_input.setText("/msg "))
        self.menu_group.clicked.connect(self.show_group_creation_dialog)
        self.menu_send_group.clicked.connect(self.show_group_message_dialog)
        self.send_button.clicked.connect(self.send_message)
        self.message_input.returnPressed.connect(self.send_message)

        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        self.connect_to_server()

    def show_group_creation_dialog(self):
        group_name, ok1 = QInputDialog.getText(self, "Create Group", "Enter group name:")
        if not ok1 or not group_name:
            return

        users, ok2 = QInputDialog.getText(self, "Add Members", "Enter usernames (comma separated):")
        if not ok2 or not users:
            return

        command = f"/newgroup {group_name} {users}"
        self.message_input.setText(command)


    def show_group_message_dialog(self):
        group_name, ok1 = QInputDialog.getText(self, "Send Group Message", "Enter group name:")
        if not ok1 or not group_name:
            return

        message, ok2 = QInputDialog.getText(self, "Message", "Enter message to send:")
        if not ok2 or not message:
            return

        command = f"/sendgroup {group_name} {message}"
        self.message_input.setText(command)





    def connect_to_server(self):
        try:
            self.client_socket.connect(('127.0.0.1', 50000))
        except Exception as e:
            self.chat_display.append(f"Connection error: {e}")
            return

        username, ok = QInputDialog.getText(self, "Username", "Enter your username:")
        if ok and username:
            self.username = username
            self.username_display.setText(f"Logged in as: {self.username}")
            encrypted_username = self.xor_encrypt_to_hex(username, SHARED_KEY)
            self.client_socket.send(encrypted_username.encode())
            threading.Thread(target=self.receive_messages, daemon=True).start()


       

    def send_message(self):
        msg = self.message_input.text()
        if msg:
            encrypted_msg = self.xor_encrypt_to_hex(msg, SHARED_KEY)
            self.client_socket.send(encrypted_msg.encode())
            self.message_input.clear()

    def receive_messages(self):
         while True:
            try:
                data = self.client_socket.recv(1024).decode()
                if data:
                    decrypted_msg = self.xor_decrypt_from_hex(data, SHARED_KEY)
                    self.chat_display.append(decrypted_msg)
            except:
                self.chat_display.append("Disconnected from server.")
                break


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = ChatClient()
    window.show()
    sys.exit(app.exec_())
