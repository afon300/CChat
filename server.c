#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "portability.h"

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048
#define PORT 8080

/* ANSI Color Codes for terminal formatting */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

/* Global data structures for managing connected clients */
socket_t client_sockets[MAX_CLIENTS];
char* client_nicknames[MAX_CLIENTS];
char* client_colors[MAX_CLIENTS];
int client_count = 0;

/* Mutex to ensure mutual exclusion when accessing shared resources */
mutex_t clients_mutex;

const char* client_color_palette[] = {
    COLOR_RED, COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_YELLOW
};
int num_colors = 5;

/**
 * Registers a new client on the server.
 * Assigns a unique color and stores the nickname.
 */
char* add_client(socket_t client_socket, char* nickname) {
    char* assigned_color = NULL;
    mutex_lock(&clients_mutex);
    
    if (client_count < MAX_CLIENTS) {
        const char* color = client_color_palette[client_count % num_colors];
        client_sockets[client_count] = client_socket;
        
        client_nicknames[client_count] = strdup(nickname); 
        if (client_nicknames[client_count] == NULL) {
            perror("Memory allocation error (strdup) for nickname");
            mutex_unlock(&clients_mutex);
            return NULL;
        }

        client_colors[client_count] = strdup(color);
        if (client_colors[client_count] == NULL) {
            perror("Memory allocation error (strdup) for color");
            free(client_nicknames[client_count]);
            mutex_unlock(&clients_mutex);
            return NULL;
        }
        assigned_color = client_colors[client_count];
        client_count++;
    } else {
        fprintf(stderr, "Maximum number of clients reached. Connection rejected.\n");
        close_socket(client_socket);
    }
    
    mutex_unlock(&clients_mutex);
    return assigned_color;
}

/**
 * Removes a client from the server and frees associated memory.
 */
void remove_client(socket_t client_socket) {
    mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (client_sockets[i] == client_socket) {
            free(client_nicknames[i]);
            free(client_colors[i]);
            
            /* Shift remaining elements in the array */
            for (int j = i; j < client_count - 1; j++) {
                client_sockets[j] = client_sockets[j + 1];
                client_nicknames[j] = client_nicknames[j + 1];
                client_colors[j] = client_colors[j + 1];
            }
            client_count--;
            break;
        }
    }
    mutex_unlock(&clients_mutex);
}

/**
 * Sends a message to all clients except the original sender.
 */
void broadcast_message(char *message, socket_t sender_socket) {
    mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (client_sockets[i] != sender_socket) {
            if (send(client_sockets[i], message, (int)strlen(message), 0) < 0) {
                perror("Broadcast message send failed");
            }
        }
    }
    mutex_unlock(&clients_mutex);
}

/**
 * Thread routine dedicated to handling a specific client.
 */
THREAD_FUNCTION handle_client(void* arg) {
    socket_t client_socket = *(socket_t*)arg;
    char buffer[BUFFER_SIZE];
    char nickname[32];
    char formatted_message[BUFFER_SIZE + 128]; 
    int bytes_received;
    char* my_color;

    /* First step: receive nickname */
    bytes_received = recv(client_socket, nickname, sizeof(nickname) - 1, 0);
    if (bytes_received <= 0) {
        if (bytes_received < 0) perror("Error during nickname reception");
        close_socket(client_socket);
        free(arg);
        THREAD_EXIT();
    }
    nickname[bytes_received] = '\0';
    
    my_color = add_client(client_socket, nickname);
    if (my_color == NULL) {
        close_socket(client_socket);
        free(arg);
        THREAD_EXIT();
    }

    /* Welcome notification */
    snprintf(formatted_message, sizeof(formatted_message), COLOR_YELLOW "[SERVER] %s joined the chat.\n" COLOR_RESET, nickname);
    printf("%s", formatted_message);
    broadcast_message(formatted_message, INVALID_SOCKET_VAL); 

    /* Main loop for receiving messages from the client */
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        snprintf(formatted_message, sizeof(formatted_message), "%s%s: " COLOR_RESET "%s", my_color, nickname, buffer);
        printf("%s", formatted_message); 
        broadcast_message(formatted_message, client_socket);
    }

    /* Cleanup after disconnection */
    if (bytes_received == 0) {
        snprintf(formatted_message, sizeof(formatted_message), COLOR_YELLOW "[SERVER] %s left the chat.\n" COLOR_RESET, nickname);
        printf("%s", formatted_message);
        broadcast_message(formatted_message, INVALID_SOCKET_VAL);
    } else {
        perror("Reception error (recv)");
    }

    remove_client(client_socket);
    close_socket(client_socket);
    free(arg);
    THREAD_EXIT();
    return THREAD_RETURN_VAL;
}

/**
 * Server entry point.
 * Initializes the listening socket and accepts incoming connections.
 */
int main() {
    init_networking();
    mutex_init(&clients_mutex);

    socket_t server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    thread_t tid;

    /* TCP socket creation */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET_VAL) {
        perror("Could not create server socket");
        cleanup_networking();
        exit(EXIT_FAILURE);
    }

    /* Address reuse option to avoid "Address already in use" errors */
    int opt = 1;
#ifdef _WIN32
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
#else
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
#endif
        perror("setsockopt failed");
        close_socket(server_socket);
        cleanup_networking();
        exit(EXIT_FAILURE);
    }

    /* Server address configuration */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close_socket(server_socket);
        cleanup_networking();
        exit(EXIT_FAILURE);
    }

    /* Listening mode */
    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        close_socket(server_socket);
        cleanup_networking();
        exit(EXIT_FAILURE);
    }

    printf("Chat server is listening on port %d...\n", PORT);

    /* Client acceptance loop */
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == INVALID_SOCKET_VAL) {
            perror("Accept failed");
            continue;
        }

        /* Allocate a descriptor for each new thread */
        socket_t *client_sock_ptr = malloc(sizeof(socket_t));
        if (client_sock_ptr == NULL) {
            perror("malloc error for client socket");
            close_socket(client_socket);
            continue;
        }
        *client_sock_ptr = client_socket;

        /* Create a thread to handle the new client */
        if (thread_create(&tid, handle_client, (void*)client_sock_ptr) != 0) {
            perror("Thread creation error");
            close_socket(client_socket);
            free(client_sock_ptr);
        } else {
            thread_detach(tid);
        }
    }

    close_socket(server_socket);
    mutex_destroy(&clients_mutex);
    cleanup_networking();
    return 0;
}
