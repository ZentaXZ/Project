CC = gcc
PKG_CONFIG = pkg-config
CFLAGS = -Wall -Wextra -std=c11 $(shell $(PKG_CONFIG) --cflags gtk4)
LDFLAGS = $(shell $(PKG_CONFIG) --libs gtk4) -lpsapi -luser32 -lkernel32

# Source files (list explicitly to handle recursive wildcards on Windows)
SRC = src/main.c \
      src/utils/json_utils.c \
      src/utils/time_utils.c \
      src/utils/uuid_utils.c \
      src/tasks/task_manager.c \
      src/tasks/task_storage.c \
      src/stats/stats.c \
      src/ui/ui_main.c \
      src/ui/ui_tasks.c \
      src/ui/ui_completed.c \
      src/ui/ui_stats.c \
      src/ui/ui_settings.c \
      src/control/process_monitor.c \
      src/control/app_blocker.c \
      src/control/schedule.c \
      src/control/reward_timer.c \
      src/control/focus_mode.c \
      src/control/soft_punishment.c \
      src/notifications/notify.c \
      third_party/cJSON/cJSON.c

OBJ = $(SRC:.c=.o)
TARGET = gestor-tareas.exe

all: setup-check $(TARGET)

setup-check:
	@echo "Checking for GTK4..."
	@$(PKG_CONFIG) --exists gtk4 || (echo "ERROR: GTK4 not found. Install with: pacman -S mingw-w64-x86_64-gtk4" && exit 1)
	@echo "✓ GTK4 found"

$(TARGET): $(OBJ)
	@echo "Linking $@..."
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)
	@echo "✓ Build successful: $(TARGET)"

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning..."
	rm -f $(OBJ) $(TARGET)
	@echo "✓ Clean complete"

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run setup-check
