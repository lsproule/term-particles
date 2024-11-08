# Makefile for MyProject

# Compiler and flags
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2 -g -Wpedantic
LDFLAGS = -lm -lncurses

# Directories
INCDIR = include
SRCDIR = src
BINDIR = bin

# Files
SRC = $(SRCDIR)/main.c
OBJ = $(SRC:.c=.o)
TARGET = $(SRCDIR)/my_project

# Rules
all: $(TARGET) $(SRCDIR)

$(TARGET): $(OBJ)
	mkdir -p $(BINDIR)
	$(CC) $(OBJ) -o $(BINDIR)/main $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o $(TARGET)

compile_commands.json: clean
	bear -- make all

.PHONY: all clean
