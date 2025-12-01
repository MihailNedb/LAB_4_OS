#include "receiver.h"

void processReadCommand(HANDLE hFile, HANDLE hMutex, HANDLE evNotEmpty, HANDLE evNotFull) {
    // Ждём доступности сообщений
    if (!waitForObject(evNotEmpty, "Waiting for messages")) {
        return;
    }

    // Захватываем мьютекс
    if (!waitForObject(hMutex, "Waiting for mutex")) {
        return;
    }

    // Читаем заголовок очереди
    QueueHeader h;
    if (!readQueueHeader(hFile, h)) {
        ReleaseMutex(hMutex);
        return;
    }

    int index = h.head;
    h.head = (h.head + 1) % h.capacity;
    h.count--;

    // Читаем сообщение
    char buf[MSG_SIZE + 1] = { 0 };
    if (!readMessage(hFile, h, index, buf)) {
        ReleaseMutex(hMutex);
        return;
    }

    // Сохраняем обновлённый заголовок
    if (!writeQueueHeader(hFile, h)) {
        ReleaseMutex(hMutex);
        return;
    }

    // Обновляем события
    if (h.count == 0) {
        ResetEvent(evNotEmpty);
    }
    SetEvent(evNotFull);

    ReleaseMutex(hMutex);
    cout << "Received: " << buf << endl;
}

void handleReceiverCommands(HANDLE hFile, HANDLE hMutex, HANDLE evNotEmpty, HANDLE evNotFull) {
    while (true) {
        cout << "Receiver command (read/exit): ";
        string cmd;
        cin >> cmd;

        if (cmd == "exit") {
            break;
        }
        else if (cmd == "read") {
            processReadCommand(hFile, hMutex, evNotEmpty, evNotFull);
        }
        else {
            cout << "Unknown command\n";
        }
    }
}

void runReceiver() {
    string filename;
    int capacity;

    cout << "Binary file name: ";
    cin >> filename;
    cout << "Number of records: ";
    cin >> capacity;

    // Создание файла и инициализация
    HANDLE hFile = openFile(filename, true);
    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }

    if (!initializeQueueFile(hFile, capacity)) {
        CloseHandle(hFile);
        return;
    }

    // Создание объектов синхронизации
    HANDLE hMutex = createMutex();
    HANDLE evNotEmpty = createEvent("QueueNotEmpty", false);
    HANDLE evNotFull = createEvent("QueueNotFull", true);

    if (!hMutex || !evNotEmpty || !evNotFull) {
        cleanupHandles({ hFile, hMutex, evNotEmpty, evNotFull });
        return;
    }

    // Запрос количества sender'ов
    int nSenders;
    cout << "Number of senders: ";
    cin >> nSenders;

    // Создаем события готовности ДО запуска процессов
    vector<HANDLE> readyEvents = createReadyEvents(nSenders);

    // Запуск Sender процессов
    vector<PROCESS_INFORMATION> processes = startAllSenders(filename, nSenders);

    // Ожидаем готовность всех sender'ов
    waitForSendersReady(readyEvents);

    // Обработка команд receiver'а
    handleReceiverCommands(hFile, hMutex, evNotEmpty, evNotFull);

    // Завершение процессов sender
    terminateAllSenders(processes);

    // Очистка ресурсов
    cleanupHandles(readyEvents);
    cleanupHandles({ hFile, hMutex, evNotEmpty, evNotFull });
}