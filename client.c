#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUFFER_SIZE 2048
#define PORT 8080

void* receive_thread(void* arg) {
    int client_socket = *(int*)arg;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer); 
        fflush(stdout);
    }

    if (bytes_received == 0) {
        printf("\n[SYSTEM] Disconnected from server.\n");
    } else {
        perror("recv");
    }
    
    close(client_socket);
    exit(0); 
    
    return NULL;
}

int start_client(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Internal error: start_client called with wrong arguments.\n");
        return 1; 
    }

    char* server_ip = argv[1];
    char* nickname = argv[2];
    
    int client_socket;
    struct sockaddr_in server_addr;
    pthread_t recv_tid;
    char buffer[BUFFER_SIZE];


    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(client_socket);
        return 1;
    }

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(client_socket);
        return 1;
    }

    printf("Connected to server as '%s'.\n", nickname);

    if (send(client_socket, nickname, strlen(nickname), 0) < 0) {
        perror("send nickname");
        close(client_socket);
        return 1;
    }

    if (pthread_create(&recv_tid, NULL, receive_thread, (void*)&client_socket) != 0) {
        perror("pthread_create");
        close(client_socket);
        return 1;
    }

    printf("You can now type messages. (Type 'exit' to quit)\n");
    while (1) {
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }

        if (strncmp(buffer, "exit\n", 5) == 0) {
            break;
        }

        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            perror("send message");
            break;
       
       
        }

}
    printf("Disconnecting...\n");
    close(client_socket);
    
    return 0;
    
}