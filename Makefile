# Makefile for Prompt Maestro - Gestor de Tareas y Productividad
# MinGW-w64 on Windows with GTK4

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g $(shell pkg-config --cflags gtk4)
LDFLAGS = $(shell pkg-config --libs gtk4) -lpsapi -luser32 -lkernel32 -lwinmm

TARGET = gestor-tareas.exe

# wildcard con /**/ no funciona en GNU Make para subcarpetas recursivas,
# por eso se usa una función recursiva explícita
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

SOURCES = $(call rwildcard,src/,*.c) third_party/cJSON/cJSON.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build successful: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete"

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run