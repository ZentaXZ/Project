# Makefile for Prompt Maestro - Gestor de Tareas y Productividad
# MinGW-w64 on Windows with GTK3

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0) -lpsapi -luser32 -lkernel32

TARGET = gestor-tareas.exe
SOURCES = $(wildcard src/**/*.c) $(wildcard src/*.c) third_party/cJSON/cJSON.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✓ Build successful: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "✓ Clean complete"

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
