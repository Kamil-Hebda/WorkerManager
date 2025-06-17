# Makefile do kompilacji projektu serwera i workera

# Kompilator C
CC = gcc

# Flagi kompilatora
CFLAGS = -Wall -Wextra -g -Iserver

# Flagi linkera (puste w tym projekcie)
LDFLAGS = 

# --- Pliki obiektowe serwera ---
SERVER_OBJ_DIR = server
SERVER_OBJS = $(SERVER_OBJ_DIR)/main_server.o \
              $(SERVER_OBJ_DIR)/task_manager.o \
              $(SERVER_OBJ_DIR)/worker_manager.o

# --- Nazwy plików wykonywalnych ---
SERVER_BIN = server_app
WORKER_BIN = worker

# --- Cele ---

.PHONY: all clean

# Cel domyślny: buduje serwer i workera
all: $(SERVER_BIN) $(WORKER_BIN)

# Cel budowania pliku wykonywalnego serwera
$(SERVER_BIN): $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) -o $@ $(LDFLAGS)

# Cel budowania pliku obiektowego main_server.o
$(SERVER_OBJ_DIR)/main_server.o: $(SERVER_OBJ_DIR)/main_server.c $(SERVER_OBJ_DIR)/task_manager.h $(SERVER_OBJ_DIR)/worker_manager.h $(SERVER_OBJ_DIR)/common_defs.h
	$(CC) $(CFLAGS) -c $< -o $@

# Cel budowania pliku obiektowego task_manager.o
$(SERVER_OBJ_DIR)/task_manager.o: $(SERVER_OBJ_DIR)/task_manager.c $(SERVER_OBJ_DIR)/task_manager.h $(SERVER_OBJ_DIR)/common_defs.h
	$(CC) $(CFLAGS) -c $< -o $@

# Cel budowania pliku obiektowego worker_manager.o
$(SERVER_OBJ_DIR)/worker_manager.o: $(SERVER_OBJ_DIR)/worker_manager.c $(SERVER_OBJ_DIR)/worker_manager.h $(SERVER_OBJ_DIR)/task_manager.h $(SERVER_OBJ_DIR)/common_defs.h
	$(CC) $(CFLAGS) -c $< -o $@

# Cel budowania pliku wykonywalnego workera
$(WORKER_BIN): worker.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Cel czyszczenia
clean:
	rm -f $(SERVER_BIN) $(WORKER_BIN)
	rm -f $(SERVER_OBJS)
