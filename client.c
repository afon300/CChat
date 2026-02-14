#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "portability.h"
#include "client.h"

#define BUFFER_SIZE 2048
#define PORT 8080

/* Mutex to synchronize display on standard output (prevents collisions) */
mutex_t stdout_mutex;

#define COLOR_GREEN   "\033[32m"
#define COLOR_RESET   "\033[0m"

/**
 * Thread responsible for continuous reception of messages from the server.
 */
THREAD_FUNCTION receive_thread(void* arg) {
    socket_t client_socket = *(socket_t*)arg;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';

        /* Critical section for display (Clear current line and print) */
        mutex_lock(&stdout_mutex);
        printf("\r\033[K"); /* Erases the local command prompt */
        printf("%s", buffer);
        printf("[YOU] /> ");
        fflush(stdout);
        mutex_unlock(&stdout_mutex);
    }

    if (bytes_received == 0) {
        mutex_lock(&stdout_mutex);
        printf("\n[SYSTEM] Disconnected from server.\n");
        mutex_unlock(&stdout_mutex);
    } else {
        perror("\n[SYSTEM] Network reception error");
    }
    
    close_socket(client_socket);
    exit(0); /* Terminate the application if the connection to the server is lost */
    
    return THREAD_RETURN_VAL;
}

/**
 * Main client logic: connection and user input loop.
 */
int start_client(int argc, char *argv[]) {
    init_networking();
    mutex_init(&stdout_mutex);

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <nickname>\n", argv[0]);
        return 1; 
    }

    char* server_ip = argv[1];
    char* nickname = argv[2];
    
    socket_t client_socket;
    struct sockaddr_in server_addr;
    thread_t recv_tid;
    char buffer[BUFFER_SIZE];

    /* TCP socket initialization */
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET_VAL) {
        perror("Failed to create socket");
        cleanup_networking();
        return 1;
    }

    /* Destination address configuration */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid or unsupported IP address.\n");
        close_socket(client_socket);
        cleanup_networking();
        return 1;
    }

    /* Attempt to connect to the server */
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close_socket(client_socket);
        cleanup_networking();
        return 1;
    }

    /* Send nickname upon connection */
    if (send(client_socket, nickname, (int)strlen(nickname), 0) < 0) {
        perror("Failed to send nickname");
        close_socket(client_socket);
        cleanup_networking();
        return 1;
    }

    /* Launch the asynchronous listening thread for incoming messages */
    if (thread_create(&recv_tid, receive_thread, (void*)&client_socket) != 0) {
        perror("Failed to create reception thread");
        close_socket(client_socket);
        cleanup_networking();
        return 1;
    }

    printf("[SYSTEM] Connected to %s:%d\n", server_ip, PORT);
    printf("[SYSTEM] Type '/exit' to quit.\n");
    printf("[YOU] /> ");
    fflush(stdout);
    
    /* Main user input loop (Blocking) */
    while (1) {
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }

        if (strncmp(buffer, "/exit\n", 6) == 0) {
            break;
        }

        /* Empty message check */
        if (strcmp(buffer, "\n") == 0) {
            printf("[YOU] /> ");
            fflush(stdout);
            continue;
        }

        /* Send message to the server */
        if (send(client_socket, buffer, (int)strlen(buffer), 0) < 0) {
            perror("Failed to send message");
            break;
        }

        /* Local visual update of the sent message */
        mutex_lock(&stdout_mutex);
        printf("\033[1A\033[K"); /* Moves up one line and clears it to format */
        printf(COLOR_GREEN "you : %s" COLOR_RESET, buffer);
        printf("[YOU] /> ");
        fflush(stdout);
        mutex_unlock(&stdout_mutex);
    }
    
    printf("[SYSTEM] Disconnecting...\n");
    close_socket(client_socket);
    mutex_destroy(&stdout_mutex);
    cleanup_networking();
    
    return 0;
}
