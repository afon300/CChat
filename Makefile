CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -g
LDFLAGS = 

# Detection of OS for shell commands
ifeq ($(OS),Windows_NT)
    MKDIR = if not exist $(1) mkdir $(1)
    RM = if exist $(1) rd /s /q $(1)
    LDFLAGS += -lws2_32
    BIN_EXT = .exe
else
    MKDIR = mkdir -p $(1)
    RM = rm -rf $(1)
    BIN_EXT =
    CFLAGS += -pthread
    LDFLAGS += -pthread
endif

# Directories
OBJ_DIR = obj
BIN_DIR = bin

# Targets
TARGET_SERVER = $(BIN_DIR)/server$(BIN_EXT)
TARGET_CLIENT = $(BIN_DIR)/main$(BIN_EXT)

# Sources
CLIENT_SRCS = main.c client.c
SERVER_SRCS = server.c

# Objects
CLIENT_OBJS = $(addprefix $(OBJ_DIR)/, $(CLIENT_SRCS:.c=.o))
SERVER_OBJS = $(addprefix $(OBJ_DIR)/, $(SERVER_SRCS:.c=.o))

all: directories $(TARGET_CLIENT) $(TARGET_SERVER)

directories:
	@$(call MKDIR, $(OBJ_DIR))
	@$(call MKDIR, $(BIN_DIR))

$(TARGET_SERVER): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_CLIENT): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: all
	@echo "Running tests..."
	@python tests/integration_test.py || python3 tests/integration_test.py || py tests/integration_test.py

clean:
	@$(call RM, $(OBJ_DIR))
	@$(call RM, $(BIN_DIR))

.PHONY: all clean directories test