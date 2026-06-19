CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = shell
SRC_DIR = src
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/utils.c $(SRC_DIR)/builtins/builtins.c $(SRC_DIR)/path.c $(SRC_DIR)/parser.c $(SRC_DIR)/builtins/auto_completion/autocompletion.c $(SRC_DIR)/builtins/complete.c $(SRC_DIR)/builtins/jobs.c
OBJS = $(SRCS:.c=.o)
HEADERS = $(SRC_DIR)/builtins/builtins.h $(SRC_DIR)/utils.h $(SRC_DIR)/path.h $(SRC_DIR)/parser.h $(SRC_DIR)/builtins/auto_completion/autocompletion.h $(SRC_DIR)/builtins/complete.h $(SRC_DIR)/builtins/jobs.h

READLINE_PREFIX := $(shell brew --prefix readline)
CFLAGS  += -I$(READLINE_PREFIX)/include
LDFLAGS  = -L$(READLINE_PREFIX)/lib -lreadline

.PHONY: all clean
all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJS) $(TARGET)