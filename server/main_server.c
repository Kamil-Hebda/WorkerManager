#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h> // Dla errno i EINTR

#include "common_defs.h"    // Definicje ogólne
#include "task_manager.h"   // Zarządzanie zadaniami
#include "worker_manager.h" // Zarządzanie workerami

// Główna funkcja serwera.
int main() {
    int server_fd;
    
    struct pollfd *client_fds = NULL; 
    WorkerInfo *worker_infos = NULL;
    int num_used_fds = 0;
    int current_capacity = 0;

    // Inicjalizacja menedżera workerów
    server_fd = WM_init_manager(&client_fds, &worker_infos);
    if (server_fd == -1) {
        fprintf(stderr, "[MAIN] Błąd inicjalizacji menedżera workerów. Zamykanie.\n");
        return EXIT_FAILURE;
    }

    // Pobranie początkowych wartości po inicjalizacji
    num_used_fds = WM_get_num_used_fds();
    current_capacity = WM_get_current_capacity();

    TM_init_tasks(); // Inicjalizacja zadań
    printf("[MAIN] System gotowy. Oczekiwanie na workerów...\n");

    // --- Główna pętla serwera ---
    while (1) {
        // Wskaźniki client_fds i worker_infos oraz liczniki num_used_fds, current_capacity
        // są modyfikowane bezpośrednio przez funkcje WM_handle_* dzięki przekazaniu ich adresów.
        // Nie ma potrzeby pobierania ich ponownie w każdej iteracji pętli.

        int poll_count = poll(client_fds, num_used_fds, -1); // Oczekiwanie na zdarzenia

        if (poll_count < 0) {
            if (errno == EINTR) { // Przerwanie przez sygnał
                continue; // Ponowienie poll()
            }
            perror("[MAIN] poll error");
            break; // Inny błąd poll, zakończenie pętli
        }

        // Iteracja po deskryptorach w poszukiwaniu zdarzeń.
        // Użycie current_loop_fds, aby uniknąć problemów przy modyfikacji num_used_fds w pętli.
        int current_loop_fds = num_used_fds; 
        for (int i = 0; i < current_loop_fds; i++) {
            if (!(client_fds[i].revents & POLLIN)) { // Brak zdarzenia POLLIN
                continue;
            }

            if (client_fds[i].fd == server_fd) { // Zdarzenie na gnieździe nasłuchującym - nowe połączenie
                int res = WM_handle_new_connection(server_fd, &client_fds, &worker_infos, &num_used_fds, &current_capacity);
                if (res == -1) {
                    fprintf(stderr, "[MAIN] Błąd obsługi nowego połączenia.\n");
                }
            } 
            else { // Zdarzenie na gnieździe workera - dane przychodzące
                int res = WM_handle_worker_data(i, &client_fds, &worker_infos, &num_used_fds, &current_capacity);
                if (res == 1) { // Worker został usunięty, tablica skompaktowana
                    i--;  // Korekta indeksu pętli
                    current_loop_fds--; // Zmniejszenie liczby deskryptorów do sprawdzenia w tej iteracji
                }
            }
        }
    }

    // --- Zwolnienie zasobów ---
    printf("[MAIN] Zamykanie serwera...\n");
    WM_cleanup_manager(server_fd, client_fds, worker_infos);
    // TM_cleanup_tasks(); // Jeśli task_manager alokowałby dynamicznie pamięć

    return 0;
}
