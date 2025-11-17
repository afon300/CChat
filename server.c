#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048
#define PORT 8080

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

int client_sockets[MAX_CLIENTS];
char* client_nicknames[MAX_CLIENTS];
char* client_colors[MAX_CLIENTS];

int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

const char* client_color_palette[] = {
    COLOR_RED, COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_YELLOW
};
int num_colors = 6;

// add client to the list
char* add_client(int client_socket, char* nickname) {

    char* assigned_color = NULL;
    pthread_mutex_lock(&clients_mutex);
    if (client_count < MAX_CLIENTS) {
        const char* color = client_color_palette[client_count % num_colors];
        client_sockets[client_count] = client_socket;
        client_nicknames[client_count] = strdup(nickname); 

        // Assign color to client
        client_colors[client_count] = strdup(color);
        assigned_color = client_colors[client_count];

        client_count++;
    } else {
        printf("Too many clients. Connection rejected.\n");
        close(client_socket);
    }
    pthread_mutex_unlock(&clients_mutex);
    return assigned_color; // Return the assigned color for further use
}


// Remove client from the list
void remove_client(int client_socket) {
    pthread_mutex_lock(&clients_mutex);
    int i;
    for (i = 0; i < client_count; i++) {
        if (client_sockets[i] == client_socket) { // Found the client to remove
            // Free allocated nickname and color
            free(client_nicknames[i]);
            free(client_colors[i]);
            

            for (int j = i; j < client_count - 1; j++) { // Shift remaining clients
                client_sockets[j] = client_sockets[j + 1];
                client_nicknames[j] = client_nicknames[j + 1];
                client_colors[j] = client_colors[j + 1];
            }
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Broadcast message to all clients except the sender
void broadcast_message(char *message, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) { // Iterate through all clients
        if (client_sockets[i] != sender_socket) { // Excpt the sender
            if (send(client_sockets[i], message, strlen(message), 0) < 0) {
                perror("send failed");
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}


void* handle_client(void* arg) {
int client_socket = *(int*)arg;
    char buffer[BUFFER_SIZE];
    char nickname[32];
    char formatted_message[BUFFER_SIZE + 100]; // Augmenter un peu la taille
    int bytes_received;
    char* my_color;


    bytes_received = recv(client_socket, nickname, 31, 0);
    nickname[bytes_received] = '\0';
    my_color = add_client(client_socket, nickname);
    
    if (my_color == NULL) {

        free(arg);
        pthread_exit(NULL);
    }

    sprintf(formatted_message, COLOR_YELLOW "[SERVER] %s has joined the chat.\n" COLOR_RESET, nickname); // <-- MODIFIER
    printf("%s", formatted_message);
    broadcast_message(formatted_message, -1); 

    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';

        sprintf(formatted_message, "%s%s: " COLOR_RESET "%s", my_color, nickname, buffer); // <-- MODIFIER

        printf("%s", formatted_message); 
        broadcast_message(formatted_message, client_socket);
    }

    if (bytes_received == 0) {
        sprintf(formatted_message, COLOR_YELLOW "[SERVER] %s has left the chat.\n" COLOR_RESET, nickname); // <-- MODIFIER
        printf("%s", formatted_message);
        broadcast_message(formatted_message, -1);
    } else {
        perror("recv error");
    }


    remove_client(client_socket);
    close(client_socket);
    free(arg);
    pthread_exit(NULL);
}

// Server main function
int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t tid;


    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }


    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }


    if (listen(server_socket, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Chat server is listening on port %d\n", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }


        int *client_sock_ptr = malloc(sizeof(int));
        if (client_sock_ptr == NULL) {
            perror("malloc");
            continue;
        }
        *client_sock_ptr = client_socket;

        if (pthread_create(&tid, NULL, handle_client, (void*)client_sock_ptr) != 0) {
            perror("pthread_create");
            free(client_sock_ptr);
        }
    }


    close(server_socket);
    return 0;
}