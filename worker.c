#include <stdio.h>       // Standardowe wejście/wyjście (printf, perror, fgets)
#include <stdlib.h>      // Standardowe funkcje ogólnego przeznaczenia (exit)
#include <string.h>      // Funkcje do manipulacji stringami i pamięcią (memset, strncpy, strncmp, strchr, strrchr, strcspn)
#include <unistd.h>      // Funkcje POSIX (close, read, sleep)
#include <sys/socket.h>  // Podstawowe definicje funkcji gniazd
#include <netinet/in.h>  // Definicje struktur adresów internetowych
#include <arpa/inet.h>   // Funkcje do konwersji adresów IP
#include <errno.h>       // Dla stałej EINTR (używanej w read_line)

#define SERVER_IP "127.0.0.1" // Adres IP serwera
#define PORT 8080             // Port serwera
#define BUFFER_SIZE 1024

// --- Funkcje pomocnicze ---

/**
 * Odczytuje jedną linię (do znaku '\n') z deskryptora pliku.
 * Odporna na fragmentację wiadomości w TCP.
 * 
 * @param fd Deskryptor pliku (gniazdo) do odczytu.
 * @param buffer Bufor, do którego zapisana zostanie linia.
 * @param n Rozmiar bufora.
 * @return Liczba odczytanych bajtów, 0 jeśli połączenie zamknięte, -1 w przypadku błędu.
 */
ssize_t read_line(int fd, void *buffer, size_t n) {
    ssize_t numRead;
    size_t totRead;
    char *buf;
    char ch;

    if (n <= 0 || buffer == NULL) {
        errno = EINVAL;
        return -1;
    }

    buf = buffer;
    totRead = 0;

    for (;;) {
        numRead = read(fd, &ch, 1); // Odczyt znaku po znaku

        if (numRead == -1) { // Błąd odczytu
            if (errno == EINTR) // Odczyt przerwany przez sygnał, ponowienie
                continue;
            else
                return -1; // Inny błąd
        } else if (numRead == 0) { // Koniec strumienia (połączenie zamknięte)
            if (totRead == 0)
                return 0;
            else
                break;
        } else { // numRead == 1 (odczytano znak)
            if (totRead < n - 1) { // Miejsce w buforze na znak i terminator null
                totRead++;
                *buf++ = ch;
            } else { // Bufor pełny. Linia obcięta. Pozostałe znaki linii pozostają w buforze gniazda.
                break; 
            }
            if (ch == '\n') { // Koniec linii
                break;
            }
        }
    }
    *buf = '\0'; // Zakończenie stringa znakiem null
    return totRead;
}

/**
 * Symuluje wykonanie zadania na podstawie jego opisu.
 * 
 * @param task_id ID zadania.
 * @param description Opis zadania.
 * @param result_buffer Bufor na wynik.
 * @param buffer_size Rozmiar bufora na wynik.
 */
void execute_task(int task_id, const char *description, char *result_buffer, int buffer_size) {
    printf("[WORKER] Wykonuję zadanie %d: '%s'\n", task_id, description);
    sleep(5); // Symulacja czasu przetwarzania
    // Symulacja zadania: Odwracanie stringa (format: "REVERSE 'string'")
    if (strncmp(description, "REVERSE '", 9) == 0) {
        const char *content_start = description + 9; // Wskaźnik za "REVERSE '"
        const char *content_end = strrchr(content_start, '\''); // Ostatni apostrof

        // Sprawdzenie formatu i czy apostrof zamykający jest na końcu
        if (content_end != NULL && *(content_end + 1) == '\0') {
            size_t content_len = content_end - content_start;
            char temp_str[256]; // Bufor na treść do odwrócenia

            if (content_len < sizeof(temp_str)) {
                strncpy(temp_str, content_start, content_len);
                temp_str[content_len] = '\0';

                // Pętla odwracająca string
                // Użycie content_len dla poprawnego odwrócenia (opis może zawierać \0).
                for (int i = 0; i < content_len / 2; i++) {
                    char temp_char = temp_str[i];
                    temp_str[i] = temp_str[content_len - 1 - i];
                    temp_str[content_len - 1 - i] = temp_char;
                }
                snprintf(result_buffer, buffer_size, "%s", temp_str);
            } else {
                snprintf(result_buffer, buffer_size, "ERROR: REVERSE content too long for internal buffer");
            }
        } else {
            snprintf(result_buffer, buffer_size, "ERROR: Invalid REVERSE format in task description (missing or misplaced end quote)");
        }
    } 
    // Symulacja zadania: Dodawanie liczb (format: "ADD X Y")
    else if (strncmp(description, "ADD ", 4) == 0) {
        int a, b;
        if (sscanf(description + 4, "%d %d", &a, &b) == 2) { // +4 dla pominięcia "ADD "
            snprintf(result_buffer, buffer_size, "%d", a + b);
        } else {
            snprintf(result_buffer, buffer_size, "ERROR: Invalid ADD format in task description");
        }
    }
    // Domyślna symulacja dla nieznanych zadań
    else {
        sleep(1); // Symulacja pracy
        snprintf(result_buffer, buffer_size, "Completed unknown task: %s", description);
    }
    printf("[WORKER] Zadanie %d zakończono.\n", task_id);
}

// --- Główna funkcja klienta (workera) ---
int main() {
    int client_fd;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    ssize_t valread;
    int keep_running = 1; // Flaga do kontrolowania głównej pętli

    // --- Inicjalizacja połączenia sieciowego ---

    // 1. Tworzenie gniazda
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation error");
        exit(EXIT_FAILURE);
    }

    // 2. Konfiguracja adresu serwera
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 3. Konwersja adresu IP z tekstu na format binarny
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        close(client_fd); // Zamknięcie gniazda przed wyjściem
        exit(EXIT_FAILURE);
    }

    // 4. Nawiązywanie połączenia z serwerem (blokujące)
    printf("[WORKER] Próbuję połączyć się z serwerem %s:%d\n", SERVER_IP, PORT);
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection failed");
        close(client_fd); // Zamknięcie gniazda przed wyjściem
        exit(EXIT_FAILURE);
    }
    printf("[WORKER] Połączono z serwerem. Rozpoczynam pracę.\n");

    // --- Główna pętla workera ---
    while (keep_running) {
        // 1. Wysłanie żądania o nowe zadanie
        if (send(client_fd, "GET_TASK\n", strlen("GET_TASK\n"), 0) < 0) {
            perror("[WORKER] send GET_TASK failed");
            keep_running = 0;
            continue;
        }
        printf("\n[WORKER] Poproszono o zadanie.\n");

        int waiting_for_task_response = 1; // Flaga pętli oczekiwania na odpowiedź serwera
        while (waiting_for_task_response && keep_running) {
            // 2. Odczyt odpowiedzi serwera
            memset(buffer, 0, BUFFER_SIZE);
            valread = read_line(client_fd, buffer, BUFFER_SIZE);

            if (valread <= 0) { // Serwer zamknął połączenie lub błąd odczytu
                if (valread == 0) {
                    printf("[WORKER] Serwer rozłączył się. Zamykam.\n");
                } else {
                    perror("[WORKER] read_line failed");
                }
                keep_running = 0; // Zakończenie głównej pętli
                waiting_for_task_response = 0; // Zakończenie pętli wewnętrznej
                continue; 
            }
            
            // Usunięcie znaku nowej linii z końca
            buffer[strcspn(buffer, "\n")] = 0; 
            if (strlen(buffer) == 0) continue; // Ignorowanie pustych linii

            printf("[SERVER->WORKER] Odebrano: '%s'\n", buffer);

            // --- Parsowanie odpowiedzi serwera ---
            if (strncmp(buffer, "TASK ", 5) == 0) {
                // Serwer przysłał zadanie
                int task_id;
                char description[256] = {0}; // Bufor na opis zadania

                // Użycie %255[^\n] zapobiega przepełnieniu bufora description
                if (sscanf(buffer, "TASK %d %255[^\n]", &task_id, description) == 2) {
                    char task_result[BUFFER_SIZE];
                    execute_task(task_id, description, task_result, BUFFER_SIZE); // Wykonanie zadania
                    
                    // Przygotowanie i wysłanie wyniku
                    char response_msg[BUFFER_SIZE];
                    snprintf(response_msg, BUFFER_SIZE, "RESULT %d %s\n", task_id, task_result);
                    
                    if (send(client_fd, response_msg, strlen(response_msg), 0) < 0) {
                        perror("[WORKER] send RESULT failed");
                        keep_running = 0; // Błąd wysyłania, zakończ
                    } else {
                        printf("[WORKER] Wysłano wynik zadania %d.\n", task_id);
                    }
                } else {
                    printf("[WORKER] Błąd parsowania zadania: %s\n", buffer);
                }
                waiting_for_task_response = 0; // Otrzymano zadanie, wyjście z pętli oczekiwania
            } else if (strncmp(buffer, "NO_TASK", 7) == 0) {
                printf("[WORKER] Brak zadań w kolejce. Czekam 3 sekundy przed kolejną prośbą...\n");
                sleep(3);
                waiting_for_task_response = 0; // Brak zadania, wyjście z pętli oczekiwania
            } else if (strncmp(buffer, "OK ", 3) == 0) { // Obsługa potwierdzeń (np. OK RESULT_RECEIVED)
                printf("[WORKER] Otrzymano potwierdzenie od serwera: '%s'.\n", buffer);
                // Potwierdzenie 'OK' nie kończy oczekiwania na główne polecenie (TASK/NO_TASK/ERROR).
            }
            else if (strncmp(buffer, "ERROR ", 6) == 0) { // Obsługa błędów od serwera
                 printf("[WORKER] Serwer zwrócił błąd: '%s'. Czekam 3 sekundy...\n", buffer);
                 sleep(3);
                 waiting_for_task_response = 0; // Otrzymano błąd, wyjście z pętli oczekiwania
            } else {
                printf("[WORKER] Odebrano nieznaną odpowiedź od serwera: '%s'. Ignoruję.\n", buffer);
                // Nieznana odpowiedź, worker kontynuuje oczekiwanie na TASK, NO_TASK lub ERROR.
            }
        } // koniec while (waiting_for_task_response)
    } // koniec while (keep_running) - główna pętla workera

    close(client_fd);
    printf("[WORKER] Połączenie zamknięte.\n");
    return 0;
}
