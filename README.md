
# 💬 Real-Time Chat Room Over TCP - ChatStreamTCP

**ChatStreamTCP** is a real-time chat room application that supports multiple users interacting concurrently over a TCP connection. The server and client components are built using multithreading to ensure efficient handling of multiple user connections, real-time message broadcasting, and automatic disconnection of inactive clients.

## 📁 **Project Overview**
This project implements a multithreaded TCP chat room server and a corresponding client. The **server** can handle multiple clients at once and broadcasts messages from any user to all other connected users in real time.

### ⚙️ **Features:**
- **Concurrent messaging**: Multiple clients can join the chat room and exchange messages simultaneously.
- **Automatic disconnection**: Clients are disconnected after a specified period of inactivity.
- **Real-time updates**: Other users are notified when a new client joins or leaves the chatroom.
- **Custom commands**: The server supports commands to list active users and exit the chatroom.

---

## 🚀 **How to Run the Project**

### **Server Application**
The **server** application is multithreaded and takes three command-line arguments:
1. **port number (P)**: The port on which the server listens.
2. **max streams (M)**: The maximum number of clients that can be connected simultaneously.
3. **timeout (T)**: The period of inactivity (in seconds) after which an inactive client will be disconnected.

To compile and run the **server**:
```bash
gcc -o server tcp_chat_server.c
./server <port> <M> <T>
```

### **Client Application**
The **client** application does not take any command-line arguments. Multiple clients can be instantiated from the same machine. Each client will prompt for a username and join the chatroom.

To compile and run the **client**:
```bash
gcc -o client tcp_chat_client.c
./client
```

---

## 🧑‍💻 **How It Works**
1. When a client connects, the **server** accepts the username, checks for uniqueness, and sends a welcome message along with the list of all currently connected users.
2. Other clients in the chatroom receive a message notifying them of the new user's arrival.
3. Clients can send messages to the server, which will broadcast the message to all other clients (except the sender).
4. Clients can issue two commands:
   - `\list`: Lists all currently logged-in users.
   - `\bye`: Disconnects the client from the server and closes their connection.
5. If a client disconnects or is inactive beyond the timeout period, the server will notify other clients that the user has left the chatroom.

---

## 🛠️ **Tech Stack**
- **C**: Programming language used for server-client implementation.
- **Multithreading**: To handle multiple client requests concurrently.
- **TCP/IP**: Reliable transport protocol used for chat communication.

---

## 📜 **Usage Example**
Here's an example to demonstrate the project in action:

1. **Running the Server**:
   ```bash
   gcc -o server tcp_chat_server.c
   ./server 8080 10 300
   ```

2. **Running Multiple Clients**:
   On different terminals (or machines), run the following:
   ```bash
   gcc -o client tcp_chat_client.c
   ./client
   ```

   Each client will connect to the server and join the chatroom. Clients can send messages, and the server will broadcast them to all other connected clients.


