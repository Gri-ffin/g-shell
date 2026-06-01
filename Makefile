CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = myshell

SRC_DIR = src

SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/builtins.c $(SRC_DIR)/path.c $(SRC_DIR)/parser.c
OBJS = $(SRCS:.c=.o)

HEADERS = $(SRC_DIR)/builtins.h $(SRC_DIR)/path.h $(SRC_DIR)/parser.h 

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
