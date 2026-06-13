CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lreadline
TARGET = shell

SRC_DIR = src

SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/utils.c $(SRC_DIR)/builtins/builtins.c $(SRC_DIR)/path.c $(SRC_DIR)/parser.c $(SRC_DIR)/builtins/autocompletion.c $(SRC_DIR)/builtins/complete.c
OBJS = $(SRCS:.c=.o)

HEADERS = $(SRC_DIR)/builtins/builtins.h $(SRC_DIR)/utils.h $(SRC_DIR)/path.h $(SRC_DIR)/parser.h $(SRC_DIR)/builtins/autocompletion.h $(SRC_DIR)/builtins/complete.h

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
