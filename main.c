#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"

/**
 * Removes the trailing newline character from a string.
 * Useful after a fgets() call.
 */
void strip_newline(char *str) {
    str[strcspn(str, "\n")] = 0;
}

/**
 * Client application entry point.
 * Handles credential acquisition and session launch.
 */
int main() {
    printf("CChat - Client Login\n");

    /* Prompt for remote server address */
    printf("Server IP address: ");
    char server_ip[100];
    if (fgets(server_ip, sizeof(server_ip), stdin) == NULL) {
        fprintf(stderr, "Error reading IP.\n");
        return 1;
    }
    strip_newline(server_ip);

    /* Prompt for user nickname */
    printf("Your nickname: ");
    char nickname[32];
    if (fgets(nickname, sizeof(nickname), stdin) == NULL) {
        fprintf(stderr, "Error reading nickname.\n");
        return 1;
    }
    strip_newline(nickname);

    /* Prepare arguments for the client module */
    char *client_args[] = { "client", server_ip, nickname };
    
    printf("Attempting to connect to %s as %s...\n", server_ip, nickname);

    /* Launch network logic */
    if (start_client(3, client_args) != 0) {
        fprintf(stderr, "Connection process failed.\n");
        return 1;
    }

    return 0;
}
