#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "common_defs.h" // Dla definicji Task

// Interfejs modułu zarządzania pulą zadań.

// Inicjuje pulę zadań.
void TM_init_tasks();

// Dodaje nowe zadanie do puli (status PENDING).
void TM_add_task_to_queue(const char *description);

// Pobiera następne zadanie (status PENDING), zmienia status na IN_PROGRESS.
// Zwraca wskaźnik do zadania lub NULL, jeśli brak zadań.
Task *TM_get_next_task();

// Znajduje zadanie po ID.
// Zwraca wskaźnik do zadania lub NULL, jeśli nie znaleziono.
Task *TM_find_task_by_id(int id);

// Zmienia status zadania z IN_PROGRESS na PENDING (re-kolejkowanie).
void TM_re_queue_task(int task_id);

#endif // TASK_MANAGER_H
