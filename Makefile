CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = myshell

# Define the source directory
SRC_DIR = src

# Prefix your files with the src/ directory
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/builtins.c $(SRC_DIR)/externals.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

# Link the object files from the src/ folder into the root executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile .c files inside src/ into .o files inside src/
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
