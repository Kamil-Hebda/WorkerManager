#ifndef WORKER_MANAGER_H
#define WORKER_MANAGER_H

#include <poll.h>          // Dla struct pollfd
#include "common_defs.h"   // Dla WorkerInfo, statusów workera

// Interfejs modułu zarządzania połączeniami workerów i ich stanem.

// Inicjuje menedżer workerów, tworzy gniazdo nasłuchujące.
// Alokuje początkowe tablice dla połączeń.
// Zwraca deskryptor gniazda nasłuchującego lub -1 w przypadku błędu.
// Aktualizuje wskaźniki fds_ptr i info_ptr.
int WM_init_manager(struct pollfd **fds_ptr, WorkerInfo **info_ptr);

// Zamyka aktywne połączenia workerów i zwalnia zaalokowaną pamięć.
void WM_cleanup_manager(int server_fd, struct pollfd *fds, WorkerInfo *info);

// Obsługuje nowe połączenie od workera.
// Akceptuje połączenie i dodaje workera do tablic monitorowania.
// Zwraca 0 (sukces) lub -1 (błąd).
// Aktualizuje wskaźniki i liczniki.
int WM_handle_new_connection(int server_fd, struct pollfd **fds_ptr, WorkerInfo **info_ptr, int *num_used_fds_ptr, int *current_capacity_ptr);

// Obsługuje dane od istniejącego workera lub jego rozłączenie.
// Zawiera logikę parsowania protokołu i re-kolejkowania zadań.
// Zwraca 1 (worker usunięty), 0 (dane obsłużone), -1 (błąd odczytu).
// Aktualizuje wskaźniki i liczniki w przypadku usunięcia.
int WM_handle_worker_data(int worker_idx, struct pollfd **fds_ptr, WorkerInfo **info_ptr, int *num_used_fds_ptr, int *current_capacity_ptr);

// Zwraca liczbę używanych deskryptorów.
int WM_get_num_used_fds();

// Zwraca wskaźnik do tablicy struct pollfd.
struct pollfd *WM_get_client_fds_ptr();

// Zwraca wskaźnik do tablicy WorkerInfo.
WorkerInfo *WM_get_worker_infos_ptr();

// Zwraca aktualną pojemność tablic.
int WM_get_current_capacity();

#endif // WORKER_MANAGER_H
