#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#define BUFFER_SIZE 4096

int *client_fds;      // Array to store client file descriptors
int *last_actives;    // Array to store last active time of clients
char **names;         // Array to store names of clients
int nofStreaming = 0; // Number of clients currently streaming
int maxConnections;   // Maximum number of clients that can connect to the server
int timeOut;

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER; // Mutex lock for client_fds array
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER; // Mutex lock for nofStreaming variable
pthread_mutex_t *lock3;                            // Mutex lock for last_actives array

void reset_string(char *string)
{
    memset(string, 0, BUFFER_SIZE);
}

void remove_trailing_new_line(char *string)
{
    int string_length = strlen(string);
    string[string_length - 1] = '\0';
}

// Broadcast message to all clients except the sender
void broadcastToAll(char message[], int except)
{
    pthread_mutex_lock(&lock2);
    for (int i = 0; i < maxConnections; i++)
    {
        if (client_fds[i] == 0 || client_fds[i] == except || names[i] == NULL)
        {
            continue;
        }
        send(client_fds[i], message, strlen(message), 0);
    }
    pthread_mutex_unlock(&lock2);
}

// Structure to pass multiple arguments to thread function
struct Values
{
    int client_fd;
    char *client_ip;
    char *name;
    int index;
};

// Function to check timeout of client
void *checkTimeout(void *obj)
{
    int index = *((int *)obj);
    pthread_mutex_lock(&(lock3[index]));
    int fd = client_fds[index];
    while (1)
    {
        int curTime = time(NULL);

        int last_access = last_actives[index];
        if (timeOut < (curTime - last_access))
        {

            if (fd == client_fds[index])
            {
                send(fd, "As you are not active in chat you are removing", 200, 0);
                /*char buf[4096];
                  sprintf(buf,"Client %s left the chat",names[index]);
                  printf("%s\n",buf);
                  broadcastToAll(buf,fd); */
                shutdown(fd, SHUT_RDWR);
            } // shutdown the socket
            pthread_mutex_unlock(&lock3[index]);
            return NULL;
        }
        else
        {
            pthread_mutex_unlock(&lock3[index]);
            sleep(timeOut - (curTime - last_access)); // sleep for remaining time
            if (fd != client_fds[index])
                return NULL; // if client already left the chat then return
            pthread_mutex_lock(&lock3[index]);
        }
    }
    return NULL;
}

// Function to handle incoming messages from clients
void *handle_incoming_message(void *index)
{

    int ind = *((int *)(index));
    int sock = client_fds[ind];
    char message[BUFFER_SIZE] = {0};

    while (client_fds[ind] > 1)
    {
        if (recv(sock, message, BUFFER_SIZE, 0) > 0)
        {
            pthread_mutex_lock(&lock3[ind]);
            last_actives[ind] = time(NULL); // update last active time of client
            // if message is \list then send list of names
            if (strcmp(message, "\\list") == 0)
            {
                char buffer[4096] = "Below are list of names who are present at the moment you asked\n";
                send(sock, buffer, strlen(buffer), 0);
                int cnt = 1;
                for (int i = 0; i < maxConnections; i++)
                {
                    if (names[i] != NULL)
                    {
                        reset_string(buffer);
                        sprintf(buffer, "%d) %s\n", cnt, names[i]);
                        cnt++;
                        send(sock, buffer, strlen(buffer), 0);
                    }
                }
            }

            // if message is \bye then inform all and close the connection
            else if (strcmp(message, "\\bye") == 0)
            {
                char buffer[4096];
                sprintf(buffer, "Ok bye %s", names[ind]);
                send(sock, buffer, strlen(buffer), 0);
                pthread_mutex_lock(&lock2);
                close(sock);
                char message2[BUFFER_SIZE];
                sprintf(message2, "Client %s left the chatroom", names[ind]);
                client_fds[ind] = 0;
                names[ind] = NULL;
                nofStreaming--;
                pthread_mutex_unlock(&lock2);

                printf("%s\n", message2);
                broadcastToAll(message2, sock);
                pthread_mutex_unlock(&lock3[ind]);
                break;
            }

            else
            {
                printf("(%d)%s:  %s\n", sock, names[ind], message);
                char message2[BUFFER_SIZE + 100];
                sprintf(message2, "(%d)%s:  %s", sock, names[ind], message);
                broadcastToAll(message2, sock);
            }

            reset_string(message);
            pthread_mutex_unlock(&lock3[ind]);
        }

        // if recv returns 0 which means client left the chat
        else
        {
            reset_string(message);
            sprintf(message, "Client %s suddenly left the chat", names[ind]);
            printf("%s\n", message);
            broadcastToAll(message, sock);
            pthread_mutex_lock(&lock2);
            close(sock);
            // update the client_fds array and names array and nofStreaming variables
            client_fds[ind] = 0;
            names[ind] = NULL;
            nofStreaming--;
            pthread_mutex_unlock(&lock2);
            break;
        }
    }
    pthread_exit(NULL);
}

// Function to setup a thread to handle incoming messages from clients
pthread_t setup_incoming_message_listener(int index)
{
    int sock = client_fds[index];
    pthread_t message_listener_id;
    //  pthread_mutex_lock(&lock2);
    if (pthread_create(&message_listener_id, NULL, handle_incoming_message, &index) != 0)
    {
        perror("[setup_incoming_message_listener] pthread_create\n");

        close(sock);
        client_fds[index] = 0;
        //  pthread_mutex_unlock(&lock2);
        exit(EXIT_FAILURE);
    }
    //  pthread_mutex_unlock(&lock2);
    return message_listener_id;
}

// Function to run client
void *runClient(void *obj)
{
    int client_fd = ((struct Values *)obj)->client_fd;
    char *client_ip = ((struct Values *)obj)->client_ip;
    int index = ((struct Values *)obj)->index;
    // take user name
    char *name = (char *)malloc(BUFFER_SIZE * sizeof(char));
    char *buffer = "Hello user please create unique userid";
    send(client_fd, buffer, strlen(buffer), 0);
    read(client_fd, name, 50);
    // check if name is already there
    pthread_mutex_lock(&lock2); // while giving name to one client , in between other clients should get
    for (int i = 0; i < maxConnections; i++)
    {
        if (client_fds[i] > 0 && i != index)
        {

            if (names[i] != NULL && strcmp(names[i], name) == 0)
            {

                char *buffer2 = "Userid already there please enter another userid";
                send(client_fd, buffer2, strlen(buffer2), 0);
                reset_string(name);
                read(client_fd, name, 50);
                i = -1;
                continue;
            }
        }
    }

    last_actives[index] = time(NULL);
    pthread_t *x = (pthread_t *)malloc(sizeof(pthread_t)); // create a thread to check timeout
    pthread_create(x, NULL, &checkTimeout, &index);
    ((struct Values *)obj)->name = name;
    char message[BUFFER_SIZE];
    sprintf(message, "Client %s joined the chatroom", name);
    printf("%s\n", message);
    names[index] = name;
    pthread_mutex_unlock(&lock2);
    broadcastToAll(message, client_fd);

    // send welcome message to client
    char welmes1[4096];
    sprintf(welmes1, "hi %s welcome to chat room\n", names[index]);
    send(client_fd, welmes1, strlen(welmes1), 0);
    char welmes[4096] = "list of live clients are | ";
    // send list of names to client
    pthread_mutex_lock(&lock2);
    for (int i = 0; i < maxConnections; i++)
    {
        if (client_fds[i] > 1 && names[i] != NULL)
        {
            strcat(welmes, names[i]);
            strcat(welmes, " | ");
        }
    }
    send(client_fd, welmes, strlen(welmes), 0);
    pthread_mutex_unlock(&lock2);
    // setup a thread to handle incoming messages from clients
    pthread_t incoming_message_listener = setup_incoming_message_listener(index);
} 

int main(int argc, char *argv[])
{
    int PORT = atoi(argv[1]);
    maxConnections = atoi(argv[2]);
    timeOut = atoi(argv[3]);
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // SOCKET - Create TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address details
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // BIND - Bind socket to address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    client_fds = (int *)calloc(maxConnections, (sizeof(int)));   // Array to store client file descriptors
    last_actives = (int *)calloc(maxConnections, (sizeof(int))); // Array to store last active time of clients
    lock3 = (pthread_mutex_t *)calloc(maxConnections, (sizeof(pthread_mutex_t)));
    for (int i = 0; i < maxConnections; i++)
    {
        pthread_mutex_init(&lock3[i], NULL);
    }

    pthread_t threads[maxConnections];
    names = (char **)malloc(maxConnections * sizeof(char *));
    for (int i = 0; i < maxConnections; i++)
    {
        names[i] = NULL;
    }
    // LISTEN - Start listening for incoming connections
    if (listen(server_fd, 100) < 0)
    {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1)
    {
        // while(nofStreaming==maxConnections)
        // {

        // }

        // ACCEPT - accept incoming connection
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Acceptance failed");
            exit(EXIT_FAILURE);
        }
        if (nofStreaming == maxConnections)
        {
            char *buffer = "Already max limit reached, so please try again later";
            send(client_fd, buffer, strlen(buffer), 0);
            close(client_fd);
            continue;
        }

        char *client_ip = malloc(sizeof(char) * (INET_ADDRSTRLEN));
        inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Client IP address: %s\n", client_ip);
        struct Values *obj = (struct Values *)malloc(sizeof(struct Values));
        obj->client_fd = client_fd;
        obj->client_ip = client_ip;

        for (int i = 0; i < maxConnections; i++)
        {
            if (client_fds[i] == 0)
            {
                client_fds[i] = client_fd;
                pthread_mutex_lock(&lock2);
                nofStreaming++;
                pthread_mutex_unlock(&lock2);
                obj->index = i;
                pthread_create(&threads[i], NULL, &runClient, obj);
                break;
            }
        }
    }

    close(server_fd);
    return 0;
}
