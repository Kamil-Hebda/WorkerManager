#include <stdio.h>
#include <string.h>
#include "task_manager.h"
#include "common_defs.h"

// Tablica zadań.
static Task all_tasks[MAX_TOTAL_TASKS];
// Licznik ID zadań.
static int next_task_id = 1;
// Liczba zadań w systemie.
static int total_tasks_count = 0;

// Inicjalizacja puli zadań (dodanie zadań testowych).
void TM_init_tasks() {
    printf("[TASK_MANAGER] Dodawanie początkowych zadań...\n");
    TM_add_task_to_queue("REVERSE 'hello world'");
    TM_add_task_to_queue("ADD 10 20");
    TM_add_task_to_queue("REVERSE 'c programming is fun'");
    TM_add_task_to_queue("ADD 50 75");
    TM_add_task_to_queue("Perform complex calculation");
    TM_add_task_to_queue("REVERSE 'distributed systems are cool'");
    TM_add_task_to_queue("ADD 123 456");
    printf("[TASK_MANAGER] Początkowe zadania dodane.\n");
}

// Dodanie nowego zadania do puli (status PENDING).
void TM_add_task_to_queue(const char *description) {
    if (total_tasks_count < MAX_TOTAL_TASKS) {
        all_tasks[total_tasks_count].id = next_task_id++;
        strncpy(all_tasks[total_tasks_count].description, description, sizeof(all_tasks[total_tasks_count].description) - 1);
        all_tasks[total_tasks_count].description[sizeof(all_tasks[total_tasks_count].description) - 1] = '\0'; // Zapewnienie null-terminacji
        all_tasks[total_tasks_count].status = TASK_STATUS_PENDING;
        printf("[TASK_MANAGER] Dodano zadanie %d: '%s' (status: PENDING)\n", all_tasks[total_tasks_count].id, all_tasks[total_tasks_count].description);
        total_tasks_count++;
    } else {
        printf("[TASK_MANAGER] Osiągnięto maksymalną liczbę zadań (%d). Nie można dodać: '%s'\n", MAX_TOTAL_TASKS, description);
    }
}

// Pobranie następnego zadania (status PENDING) i zmiana statusu na IN_PROGRESS.
Task *TM_get_next_task() {
    for (int i = 0; i < total_tasks_count; i++) {
        if (all_tasks[i].status == TASK_STATUS_PENDING) {
            all_tasks[i].status = TASK_STATUS_IN_PROGRESS;
            printf("[TASK_MANAGER] Przydzielono zadanie %d: '%s' (status: IN_PROGRESS)\n", all_tasks[i].id, all_tasks[i].description);
            return &all_tasks[i];
        }
    }
    return NULL; // Brak zadań oczekujących
}

// Wyszukanie zadania po ID.
Task *TM_find_task_by_id(int id) {
    for (int i = 0; i < total_tasks_count; i++) {
        if (all_tasks[i].id == id) {
            return &all_tasks[i];
        }
    }
    return NULL; // Nie znaleziono zadania
}

// Zmiana statusu zadania z IN_PROGRESS na PENDING.
void TM_re_queue_task(int task_id) {
    Task *task = TM_find_task_by_id(task_id);
    if (task != NULL && task->status == TASK_STATUS_IN_PROGRESS) {
        task->status = TASK_STATUS_PENDING;
        printf("[TASK_MANAGER] Zadanie %d ('%s') ponownie w kolejce (status: PENDING).\n", task->id, task->description);
    } else if (task != NULL) {
        printf("[TASK_MANAGER] Ostrzeżenie: Próba re-kolejkowania zadania %d (status: %d), nie jest IN_PROGRESS.\n", task_id, task->status);
    } else {
        printf("[TASK_MANAGER] Błąd: Nie znaleziono zadania ID %d do re-kolejkowania.\n", task_id);
    }
}
