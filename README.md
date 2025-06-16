
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

Aby skompilować projekt, użyj kompilatora `gcc`.

```bash
# Kompilacja serwera
gcc server.c -o server 

# Kompilacja workera
gcc worker.c -o worker worker.c
```

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

Projekt składa się z dwóch plików źródłowych C:

*   `server.c`: Implementacja serwera kolejki zadań.
*   `worker.c`: Implementacja klienta pełniącego rolę workera.

**Protokół Aplikacji:**

System używa prostego protokołu tekstowego opartego na liniach zakończonych znakiem nowej linii (`\n`). Kluczowe komendy to:

*   **`GET_TASK`**: Worker prosi serwer o nowe zadanie.
*   **`TASK <ID> <Opis>`**: Serwer przydziela zadanie o danym ID i opisie.
*   **`NO_TASK`**: Serwer informuje, że nie ma dostępnych zadań.
*   **`RESULT <ID> <Wynik>`**: Worker odsyła wynik wykonanego zadania.
*   **`OK <Opis>`**: Serwer potwierdza pomyślne wykonanie operacji (np. `OK RESULT_RECEIVED`).
*   **`ERROR <Opis_Błędu>`**: Serwer zgłasza błąd.
