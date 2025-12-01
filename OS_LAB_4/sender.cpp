#include "sender.h"

void processSendCommand(HANDLE hFile, HANDLE hMutex, HANDLE evNotFull, HANDLE evNotEmpty) {
    cout << "Message: ";
    string msg;
    cin.ignore();
    getline(cin, msg);

    if (msg.size() > MSG_SIZE) {
        msg.resize(MSG_SIZE);
    }

    // Ждём места в очереди
    if (!waitForObject(evNotFull, "Waiting for space in queue")) {
        return;
    }

    // Захватываем мьютекс
    if (!waitForObject(hMutex, "Waiting for mutex")) {
        return;
    }

    // Читаем и обновляем заголовок очереди
    QueueHeader q;
    if (!readQueueHeader(hFile, q)) {
        ReleaseMutex(hMutex);
        return;
    }

    int index = q.tail;
    q.tail = (q.tail + 1) % q.capacity;
    q.count++;

    // Записываем сообщение
    if (!writeMessage(hFile, q, index, msg)) {
        ReleaseMutex(hMutex);
        return;
    }

    // Сохраняем обновлённый заголовок
    if (!writeQueueHeader(hFile, q)) {
        ReleaseMutex(hMutex);
        return;
    }

    ReleaseMutex(hMutex);
    SetEvent(evNotEmpty);

    cout << "Message sent successfully\n";
}

void handleSenderCommands(HANDLE hFile, HANDLE hMutex, HANDLE evNotFull, HANDLE evNotEmpty) {
    while (true) {
        cout << "Sender command (send/exit): ";
        string cmd;
        cin >> cmd;

        if (cmd == "exit") {
            break;
        }
        else if (cmd == "send") {
            processSendCommand(hFile, hMutex, evNotFull, evNotEmpty);
        }
        else {
            cout << "Unknown command\n";
        }
    }
}

void runSender(string filename, int senderId) {
    cout << "Sender #" << senderId << " starting...\n";

    // Ждем немного чтобы файл успел создаться
    Sleep(1000);

    // Открываем бинарный файл
    HANDLE hFile = openFile(filename);
    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }

    // Открываем объекты синхронизации
    HANDLE hMutex = openMutex();
    if (!hMutex) {
        CloseHandle(hFile);
        return;
    }

    HANDLE evNotEmpty = openEvent("QueueNotEmpty");
    HANDLE evNotFull = openEvent("QueueNotFull");

    // Сигнал о готовности
    signalSenderReady(senderId);

    // Обработка команд
    handleSenderCommands(hFile, hMutex, evNotFull, evNotEmpty);

    // Очистка ресурсов
    vector<HANDLE> handles = { hFile, hMutex, evNotEmpty, evNotFull };
    cleanupHandles(handles);
}