# Makefile for MyProject

# Compiler and flags
CC = gcc
CFLAGS = -std=c23 -Wall -Wextra -pedantic -O2 -g -Wpedantic
LDFLAGS = -L/usr/lib64/libncursesw.so -lm -lncursesw

# Directories
INCDIR = include
SRCDIR = src
BINDIR = bin

# Files
SRC = $(SRCDIR)/main.c
OBJ = $(SRC:.c=.o)
TARGET = $(BINDIR)/my_project

# Rules
all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p $(BINDIR)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o $(TARGET)

compile_commands.json: clean
	bear -- make all

.PHONY: all clean
