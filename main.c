#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"

void strip_newline(char *str) {

    str[strcspn(str, "\n")] = 0;
}

int main() {
    printf("Client login\n");

    printf("Enter server IP: ");
    char server_ip[100];
    if (fgets(server_ip, sizeof(server_ip), stdin) == NULL) {
        fprintf(stderr, "Error reading server IP.\n");
        return 1;
    }
    strip_newline(server_ip);

    printf("Enter your nickname: ");
    char nickname[32];
    if (fgets(nickname, sizeof(nickname), stdin) == NULL) {
        fprintf(stderr, "Error reading nickname.\n");
        return 1;
    }
    strip_newline(nickname);

    char *client_args[] = { "client", server_ip, nickname };
    
    printf("Attempting to connect to %s as %s...\n", server_ip, nickname);

    if (start_client(3, client_args) != 0) {
        fprintf(stderr, "Client connection process failed.\n");
        return 1;
    }

    return 0;
}