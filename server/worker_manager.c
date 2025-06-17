#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#include "worker_manager.h" // Nagłówek modułu
#include "task_manager.h"   // Zarządzanie zadaniami
#include "common_defs.h"    // Definicje ogólne

// --- Zmienne globalne modułu ---
// Wskaźniki do dynamicznych tablic przechowywanych i zarządzanych przez ten moduł.
static struct pollfd *client_fds = NULL; // Tablica deskryptorów plików dla poll()
static WorkerInfo *worker_infos = NULL;  // Tablica informacji o workerach
// Aktualna zaalokowana pojemność tablic.
static int current_capacity = 0;
// Liczba faktycznie używanych deskryptorów/workerów.
static int num_used_fds = 0;

// --- Implementacja funkcji menedżera workerów ---

// Inicjuje menedżer workerów, tworzy gniazdo nasłuchujące i alokuje początkowe tablice dla połączeń.
int WM_init_manager(struct pollfd **fds_ptr, WorkerInfo **info_ptr) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // Tworzenie i konfiguracja gniazda nasłuchującego
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[WM] socket failed");
        return -1;
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("[WM] setsockopt");
        close(server_fd);
        return -1;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[WM] bind failed");
        close(server_fd);
        return -1;
    }
    if (listen(server_fd, 10) < 0) {
        perror("[WM] listen");
        close(server_fd);
        return -1;
    }
    printf("[WM] Serwer nasłuchuje na porcie %d\n", PORT);

    // Alokacja pamięci dla tablic połączeń i informacji o workerach
    current_capacity = INITIAL_CAPACITY;
    client_fds = (struct pollfd *)malloc(current_capacity * sizeof(struct pollfd));
    worker_infos = (WorkerInfo *)malloc(current_capacity * sizeof(WorkerInfo));
    if (client_fds == NULL || worker_infos == NULL) {
        perror("[WM] Initial malloc for client_fds or worker_infos failed");
        close(server_fd);
        if (client_fds) free(client_fds); // Zwolnienie w przypadku częściowego błędu
        if (worker_infos) free(worker_infos);
        return -1;
    }

    // Inicjalizacja slotów w tablicach jako wolne
    for (int i = 0; i < current_capacity; i++) {
        client_fds[i].fd = -1;
        worker_infos[i].fd = -1;
    }

    // Dodanie gniazda nasłuchującego do monitorowania przez poll()
    client_fds[0].fd = server_fd;
    client_fds[0].events = POLLIN;
    num_used_fds = 1;

    // Przekazanie wskaźników do zaalokowanej pamięci
    *fds_ptr = client_fds;
    *info_ptr = worker_infos;

    return server_fd;
}

// Czyszczenie zasobów menedżera workerów.
void WM_cleanup_manager(int server_fd, struct pollfd *fds, WorkerInfo *info) {
    printf("[WM] Zamykanie menedżera workerów...\n");
    if (fds != NULL) {
        // Zamknięcie gniazd klientów
        for (int i = 0; i < num_used_fds; i++) {
            if (fds[i].fd != -1 && fds[i].fd != server_fd) {
                close(fds[i].fd);
            }
        }
        free(fds);
        client_fds = NULL;
    }
    if (info != NULL) {
        free(info);
        worker_infos = NULL;
    }
    close(server_fd); // Zamknięcie gniazda nasłuchującego
    printf("[WM] Menedżer workerów zamknięty.\n");
}

// Obsługa nowego połączenia.
int WM_handle_new_connection(int server_fd, struct pollfd **fds_ptr, WorkerInfo **info_ptr, int *num_used_fds_ptr, int *current_capacity_ptr) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    // Akceptacja nowego połączenia
    int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    if (new_socket < 0) {
        perror("[WM] accept error");
        return -1;
    }

    printf("[WM] Nowy worker połączył się: deskryptor %d\n", new_socket);

    // Sprawdzenie pojemności tablic i ewentualna realokacja
    if (*num_used_fds_ptr == *current_capacity_ptr) {
        printf("[WM] Tablica klientów pełna (%d/%d). Reallocating...\n", *num_used_fds_ptr, *current_capacity_ptr);
        
        int old_capacity = *current_capacity_ptr;
        *current_capacity_ptr += REALLOC_INCREMENT; // Zwiększenie pojemności

        // Realokacja tablicy pollfd
        struct pollfd *temp_fds = (struct pollfd *)realloc(*fds_ptr, *current_capacity_ptr * sizeof(struct pollfd));
        if (temp_fds == NULL) {
            perror("[WM] realloc client_fds failed, cannot add new client");
            close(new_socket);
            *current_capacity_ptr = old_capacity; // Przywrócenie starej pojemności
            return -1;
        }
        *fds_ptr = temp_fds;

        // Realokacja tablicy WorkerInfo
        WorkerInfo *temp_worker_infos = (WorkerInfo *)realloc(*info_ptr, *current_capacity_ptr * sizeof(WorkerInfo));
        if (temp_worker_infos == NULL) {
            perror("[WM] realloc worker_infos failed, cannot add new client");
            close(new_socket);
            *current_capacity_ptr = old_capacity; // Przywrócenie starej pojemności
            return -1;
        }
        *info_ptr = temp_worker_infos;

        // Inicjalizacja nowo zaalokowanych miejsc
        for (int k = old_capacity; k < *current_capacity_ptr; k++) {
            (*fds_ptr)[k].fd = -1;
            (*info_ptr)[k].fd = -1;
        }
    }
    
    // Dodanie nowego gniazda workera do monitorowania
    (*fds_ptr)[*num_used_fds_ptr].fd = new_socket;
    (*fds_ptr)[*num_used_fds_ptr].events = POLLIN;

    // Inicjalizacja informacji o nowym workerze
    (*info_ptr)[*num_used_fds_ptr].fd = new_socket;
    (*info_ptr)[*num_used_fds_ptr].status = WORKER_STATUS_IDLE;
    (*info_ptr)[*num_used_fds_ptr].current_task_id = -1;

    (*num_used_fds_ptr)++; // Zwiększenie licznika używanych deskryptorów

    return 0; // Sukces
}

// Obsługuje dane od istniejącego workera lub jego rozłączenie.
int WM_handle_worker_data(int worker_idx, struct pollfd **fds_ptr, WorkerInfo **info_ptr, int *num_used_fds_ptr, int *current_capacity_ptr) {
    char buffer[BUFFER_SIZE];
    // Odczyt danych z gniazda
    int valread = read((*fds_ptr)[worker_idx].fd, buffer, BUFFER_SIZE - 1);

    // Obsługa rozłączenia lub błędu odczytu
    if (valread <= 0) {
        if (valread == 0) { // Klient się rozłączył
            printf("[WM] Worker (deskryptor %d) rozłączył się.\n", (*fds_ptr)[worker_idx].fd);
        } else { // Błąd odczytu
            perror("[WM] read error");
            printf("[WM] Błąd odczytu na deskryptorze %d. Zamykam połączenie.\n", (*fds_ptr)[worker_idx].fd);
        }

        // Re-kolejkowanie zadania, jeśli worker był zajęty
        if ((*info_ptr)[worker_idx].status == WORKER_STATUS_BUSY && (*info_ptr)[worker_idx].current_task_id != -1) {
            printf("[WM] Worker %d (fd %d) rozłączył się w trakcie zadania %d. Próba re-kolejkowania.\n",
                    (*info_ptr)[worker_idx].fd, (*fds_ptr)[worker_idx].fd, (*info_ptr)[worker_idx].current_task_id);
            TM_re_queue_task((*info_ptr)[worker_idx].current_task_id);
        }

        close((*fds_ptr)[worker_idx].fd); // Zamknięcie gniazda

        // Kompaktowanie tablic (przesunięcie ostatniego elementu)
        if (worker_idx != *num_used_fds_ptr - 1) {
            (*fds_ptr)[worker_idx] = (*fds_ptr)[*num_used_fds_ptr - 1];
            (*info_ptr)[worker_idx] = (*info_ptr)[*num_used_fds_ptr - 1];
        }
        (*num_used_fds_ptr)--;

        // Opcjonalne zmniejszanie alokacji pamięci
        if (*num_used_fds_ptr < *current_capacity_ptr - REALLOC_INCREMENT && *current_capacity_ptr > INITIAL_CAPACITY) {
            printf("[WM] Zmniejszam alokację tablic: %d -> %d\n", *current_capacity_ptr, *current_capacity_ptr - REALLOC_INCREMENT);
            *current_capacity_ptr -= REALLOC_INCREMENT;
            
            struct pollfd *temp_fds_shrink = (struct pollfd *)realloc(*fds_ptr, *current_capacity_ptr * sizeof(struct pollfd));
            WorkerInfo *temp_worker_infos_shrink = (WorkerInfo *)realloc(*info_ptr, *current_capacity_ptr * sizeof(WorkerInfo));
            
            if (temp_fds_shrink == NULL || temp_worker_infos_shrink == NULL) {
                perror("[WM] realloc (shrink) failed, continuing with larger array");
                *current_capacity_ptr += REALLOC_INCREMENT; // Przywrócenie pojemności w razie błędu
            } else {
                *fds_ptr = temp_fds_shrink;
                *info_ptr = temp_worker_infos_shrink;
            }
        }
        return 1; // Sygnalizacja usunięcia workera
    } else { // Odebrano dane od workera (valread > 0)
        buffer[valread] = '\0'; // Zakończenie stringa
        printf("[WM] Odebrano od workera %d: '%s'\n", (*fds_ptr)[worker_idx].fd, buffer);

        // --- Parsowanie i obsługa komend ---
        // Komenda: GET_TASK
        if (strncmp(buffer, "GET_TASK", 8) == 0 && (buffer[8] == '\n' || buffer[8] == '\0')) {
            if ((*info_ptr)[worker_idx].status == WORKER_STATUS_IDLE) {
                Task *task = TM_get_next_task(); // Pobranie zadania
                if (task != NULL) {
                    (*info_ptr)[worker_idx].status = WORKER_STATUS_BUSY;
                    (*info_ptr)[worker_idx].current_task_id = task->id;

                    char response[BUFFER_SIZE];
                    snprintf(response, BUFFER_SIZE, "TASK %d %s\n", task->id, task->description);
                    send((*fds_ptr)[worker_idx].fd, response, strlen(response), 0);
                    printf("[WM] Przydzielono zadanie %d ('%s') workerowi %d (fd %d).\n", 
                            task->id, task->description, (*info_ptr)[worker_idx].fd, (*fds_ptr)[worker_idx].fd);
                } else { // Brak zadań
                    send((*fds_ptr)[worker_idx].fd, "NO_TASK\n", strlen("NO_TASK\n"), 0);
                    printf("[WM] Brak zadań w kolejce dla workera %d (fd %d).\n", (*info_ptr)[worker_idx].fd, (*fds_ptr)[worker_idx].fd);
                }
            } else { // Worker zajęty
                send((*fds_ptr)[worker_idx].fd, "ERROR ALREADY_BUSY\n", strlen("ERROR ALREADY_BUSY\n"), 0);
                printf("[WM] Worker %d (fd %d) jest już zajęty.\n", (*info_ptr)[worker_idx].fd, (*fds_ptr)[worker_idx].fd);
            }
        } 
        // Komenda: RESULT
        else if (strncmp(buffer, "RESULT ", 7) == 0) {
            int task_id;
            char result_str[BUFFER_SIZE]; 
            // Parsowanie ID zadania i wyniku, z ograniczeniem długości
            if (sscanf(buffer, "RESULT %d %1023[^\n]", &task_id, result_str) == 2) {
                printf("[WM] Odebrano wynik od workera %d (fd %d) dla zadania %d: '%s'\n", 
                        (*info_ptr)[worker_idx].fd, (*fds_ptr)[worker_idx].fd, task_id, result_str);
                
                // Weryfikacja ID zadania i statusu workera
                if ((*info_ptr)[worker_idx].status == WORKER_STATUS_BUSY && (*info_ptr)[worker_idx].current_task_id == task_id) {
                    printf("[WM] Zadanie %d zakończone przez workera %d. Wynik: '%s'\n", task_id, (*info_ptr)[worker_idx].fd, result_str);
                    (*info_ptr)[worker_idx].status = WORKER_STATUS_IDLE;
                    (*info_ptr)[worker_idx].current_task_id = -1;
                    
                    Task *completed_task = TM_find_task_by_id(task_id);
                    if (completed_task != NULL) {
                        completed_task->status = TASK_STATUS_COMPLETED;
                        printf("[TASK_MANAGER] Zadanie %d ('%s') status: COMPLETED.\n", completed_task->id, completed_task->description);
                    } else {
                        printf("[WM] Ostrzeżenie: Wynik dla zadania %d, nie znaleziono w puli.\n", task_id);
                    }

                    send((*fds_ptr)[worker_idx].fd, "OK RESULT_RECEIVED\n", strlen("OK RESULT_RECEIVED\n"), 0);
                } else {
                    send((*fds_ptr)[worker_idx].fd, "ERROR INVALID_TASK_ID_OR_NOT_BUSY\n", strlen("ERROR INVALID_TASK_ID_OR_NOT_BUSY\n"), 0);
                    printf("[WM] Błąd: Worker %d (fd %d) odesłał wynik dla niepoprawnego zadania %d (oczekiwano %d).\n", 
                            (*info_ptr)[worker_idx].fd, (*fds_ptr)[worker_idx].fd, task_id, (*info_ptr)[worker_idx].current_task_id);
                }
            } else {
                send((*fds_ptr)[worker_idx].fd, "ERROR INVALID_RESULT_FORMAT\n", strlen("ERROR INVALID_RESULT_FORMAT\n"), 0);
                printf("[WM] Błąd: Nieprawidłowy format RESULT od workera %d (fd %d): '%s'\n", worker_idx, (*fds_ptr)[worker_idx].fd, buffer);
            }
        } 
        // Nieznana komenda
        else {
            printf("[WM] Odebrano nieznaną komendę od deskryptora %d: '%s'\n", (*fds_ptr)[worker_idx].fd, buffer);
            send((*fds_ptr)[worker_idx].fd, "ERROR UNKNOWN_COMMAND\n", strlen("ERROR UNKNOWN_COMMAND\n"), 0);
        }
    }
    return 0; // Sukces, worker aktywny
}

// Gettery dla zmiennych statycznych.
int WM_get_num_used_fds() {
    return num_used_fds;
}

struct pollfd *WM_get_client_fds_ptr() {
    return client_fds;
}

WorkerInfo *WM_get_worker_infos_ptr() {
    return worker_infos;
}

int WM_get_current_capacity() {
    return current_capacity;
}
