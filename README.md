# Prosty System Rozproszonej Kolejki Zadań (WorkerManager)

## Opis Projektu

Ten projekt implementuje prosty system rozproszonej kolejki zadań, składający się z centralnego serwera i wielu klientów pełniących rolę "workerów". Głównym celem systemu jest demonstracja efektywnej obsługi wielu współbieżnych klientów oraz mechanizmu niezawodnego przydzielania i re-kolejkowania zadań.

**Kluczowe funkcjonalności:**

*   **Serwer:** Zarządza pulą zadań i ich statusami. Przydziela zadania wolnym workerom.
*   **Workerzy:** Łączą się z serwerem, pobierają zadania, symulują ich wykonanie (z opóźnieniem) i odsyłają wyniki.
*   **Współbieżna Obsługa Klientów:** Serwer wykorzystuje mechanizm I/O multiplexingu (`poll()`) do nieblokującej obsługi wielu workerów jednocześnie.
*   **Niezawodne Przydzielanie Zadań:** System wspiera automatyczne re-kolejkowanie zadań, jeśli worker rozłączy się w trakcie ich wykonywania, zapewniając, że żadne zadanie nie zostanie utracone.

## Architektura i Technologie

*   **Język programowania:** C
*   **Model komunikacji:** Klient-Serwer
*   **Protokół transportowy:** TCP/IP (dla niezawodności i połączeniowości)
*   **Zarządzanie połączeniami:** `poll()` (I/O multiplexing)
*   **Protokół aplikacji:** Prosty, tekstowy protokół typu żądanie-odpowiedź.

## Kompilacja

Aby skompilować projekt, użyj dostarczonego pliku `Makefile`. Makefile automatyzuje proces kompilacji serwera i workera.

```bash
# Kompilacja całego projektu (serwer i worker)
make

# Kompilacja samego serwera
make server

# Kompilacja samego workera
make worker

# Usunięcie skompilowanych plików
make clean
```
Upewnij się, że masz zainstalowane narzędzie `make` oraz kompilator `gcc`.

## Uruchamianie

### 1. Uruchamianie Serwera

Otwórz terminal i uruchom program `server`.

```bash
./server
```

Serwer zacznie nasłuchiwać na porcie `8080`. W logach serwera zobaczysz komunikaty o dodawaniu początkowych zadań do kolejki i oczekiwaniu na połączenia workerów.

```
[SERVER] Serwer nasłuchuje na porcie 8080
[SERVER] Dodawanie początkowych zadań do kolejki...
[TASK_QUEUE] Dodano zadanie 1: 'REVERSE 'hello world''
...
[SERVER] Kolejka zadań przygotowana. Oczekuję na workerów...
```

### 2. Uruchamianie Workerów

Otwórz **nowe terminale** dla każdego workera, którego chcesz uruchomić.

```bash
./worker
```

Każdy worker połączy się z serwerem, będzie prosił o zadania, symulował ich wykonanie (z krótkim opóźnieniem dzięki `sleep()`) i odsyłał wyniki.

**Przykładowe logi workera:**

```
[WORKER] Próbuję połączyć się z serwerem 127.0.0.1:8080
[WORKER] Połączono z serwerem. Rozpoczynam pracę.

[WORKER] Poproszono o zadanie.
[SERVER->WORKER] Odebrano: 'TASK 1 REVERSE 'hello world''
[WORKER] Wykonuję zadanie 1: 'REVERSE 'hello world''
[WORKER] Zadanie 1 zakończono.
[WORKER] Wysłano wynik zadania 1.
[SERVER->WORKER] Odebrano: 'OK RESULT_RECEIVED'
[WORKER] Otrzymano potwierdzenie od serwera: 'OK RESULT_RECEIVED'. Czekam na nowe zadanie...
```

**Przykładowe logi serwera (gdy workery się łączą i pracują):**

```
[SERVER] Nowy worker połączył się: deskryptor 4
[SERVER] Odebrano od workera 4: 'GET_TASK'
[SERVER] Przydzielono zadanie 1 ('REVERSE 'hello world'') workerowi 4 (fd 4).
[SERVER] Odebrano od workera 4: 'RESULT 1 'dlrow olleh''
[SERVER] Zadanie 1 zakończone przez workera 4. Wynik: 'dlrow olleh'
[TASK_QUEUE] Zadanie 1 ('REVERSE 'hello world'') status zmieniony na COMPLETED.
```

## Kod i Struktura Projektu

Projekt jest zorganizowany w następujący sposób:

*   **`Makefile`**: Skrypt automatyzujący proces kompilacji i czyszczenia projektu.
*   **`worker.c`**: Implementacja klienta (workera), który łączy się z serwerem, pobiera i wykonuje zadania.
*   **`server/`**: Katalog zawierający kod źródłowy serwera.
    *   **`main_server.c`**: Główny plik serwera, odpowiedzialny za inicjalizację, główną pętlę obsługi zdarzeń (`poll()`) oraz koordynację modułów.
    *   **`worker_manager.h`** i **`worker_manager.c`**: Moduł zarządzający połączeniami od workerów. Odpowiada za akceptowanie nowych połączeń, obsługę danych przychodzących od workerów, zarządzanie tablicami deskryptorów plików (`pollfd`) i informacji o workerach (`WorkerInfo`), a także za re-kolejkowanie zadań w przypadku rozłączenia workera.
    *   **`task_manager.h`** i **`task_manager.c`**: Moduł zarządzający pulą zadań. Odpowiada za przechowywanie zadań, ich dodawanie, wyszukiwanie, przydzielanie workerom oraz aktualizację statusów zadań.
    *   **`common_defs.h`**: Plik nagłówkowy zawierający wspólne definicje (stałe, struktury danych, statusy) używane przez różne moduły serwera oraz potencjalnie przez workera.

**Protokół Aplikacji:**

System używa prostego protokołu tekstowego opartego na liniach zakończonych znakiem nowej linii (`\n`). Kluczowe komendy to:

*   **`GET_TASK`**: Worker prosi serwer o nowe zadanie.
*   **`TASK <ID> <Opis>`**: Serwer przydziela zadanie o danym ID i opisie.
*   **`NO_TASK`**: Serwer informuje, że nie ma dostępnych zadań.
*   **`RESULT <ID> <Wynik>`**: Worker odsyła wynik wykonanego zadania.
*   **`OK <Opis>`**: Serwer potwierdza pomyślne wykonanie operacji (np. `OK RESULT_RECEIVED`).
*   **`ERROR <Opis_Błędu>`**: Serwer zgłasza błąd.
