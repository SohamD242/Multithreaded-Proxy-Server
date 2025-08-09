# Client code for a simple socket communication example
import socket
import threading
import time

# if you want to send objects, you can use pickle or json for serialization

HEADER = 64
FORMAT = 'utf-8'
PORT = 5050
DISCONNECT_MESSAGE = "!DISCONNECT"
SERVER = socket.gethostbyname(socket.gethostname())
ADDR = (SERVER, PORT)
# print(f"[SERVER] Server IP: {SERVER}")

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM) #SOCK_DGRAM - UDP
client.connect(ADDR)

def send(msg):
    message = msg.encode(FORMAT)
    msg_length = len(message)
    send_length = str(msg_length).encode(FORMAT)
    send_length += b' ' * (HEADER - len(send_length))
    client.send(send_length)
    client.send(message)
    print(client.recv(2048).decode(FORMAT))  # Wait for server acknowledgment
    
send("Hello World!")
input("Press Enter to send another message...")
send("Hello Soham!")
input("Press Enter to send another message...")
send("Hola Mundo!")
send("Bonjour le monde!")
input("Press Enter to disconnect...")
send(DISCONNECT_MESSAGE)