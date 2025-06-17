#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

// Stałe konfiguracyjne.
#define PORT 8080             // Port serwera
#define BUFFER_SIZE 1024      // Rozmiar bufora odczytu/zapisu
#define INITIAL_CAPACITY 5    // Początkowa pojemność tablic dynamicznych
#define REALLOC_INCREMENT 5   // Krok zwiększania pojemności tablic
#define MAX_TOTAL_TASKS 100   // Maksymalna liczba zadań w systemie

// Statusy zadań.
#define TASK_STATUS_PENDING      0 // Oczekujące
#define TASK_STATUS_IN_PROGRESS  1 // W trakcie realizacji
#define TASK_STATUS_COMPLETED    2 // Zakończone
#define TASK_STATUS_FAILED       3 // Nieudane

// Statusy workerów.
#define WORKER_STATUS_IDLE 0 // Dostępny
#define WORKER_STATUS_BUSY 1 // Zajęty

// Struktura zadania.
typedef struct {
    int id;
    char description[256];
    int status;
} Task;

// Struktura informacji o workerze.
typedef struct {
    int fd;                 // Deskryptor gniazda workera
    int status;             // Aktualny status workera
    int current_task_id;    // ID zadania aktualnie przetwarzanego (-1 jeśli brak)
} WorkerInfo;

#endif // COMMON_DEFS_H
