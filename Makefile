CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -pthread

TARGET_SERVER = server
TARGET_CLIENT = main

CLIENT_SRCS = main.c client.c

all: $(TARGET_CLIENT) $(TARGET_SERVER)

$(TARGET_SERVER): server.c
	$(CC) $(CFLAGS) server.c -o $(TARGET_SERVER) $(LDFLAGS)

$(TARGET_CLIENT): $(CLIENT_SRCS)
	$(CC) $(CFLAGS) $(CLIENT_SRCS) -o $(TARGET_CLIENT) $(LDFLAGS)

clean:
	rm -f $(TARGET_SERVER) $(TARGET_CLIENT)

.PHONY: all clean