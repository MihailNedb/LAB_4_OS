#include "sender.h"

void processSendCommand(HANDLE hFile, HANDLE hMutex, HANDLE evNotFull, HANDLE evNotEmpty) {
    cout << "Message: ";
    string msg;
    cin.ignore();
    getline(cin, msg);

    if (msg.size() > MSG_SIZE) {
        msg.resize(MSG_SIZE);
    }


    if (!waitForObject(evNotFull, "Waiting for space in queue")) {
        return;
    }

    if (!waitForObject(hMutex, "Waiting for mutex")) {
        return;
    }

    QueueHeader q;
    if (!readQueueHeader(hFile, q)) {
        ReleaseMutex(hMutex);
        return;
    }

    int index = q.tail;
    q.tail = (q.tail + 1) % q.capacity;
    q.count++;

    if (!writeMessage(hFile, q, index, msg)) {
        ReleaseMutex(hMutex);
        return;
    }

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

    Sleep(1000);

    HANDLE hFile = openFile(filename);
    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }

    HANDLE hMutex = openMutex();
    if (!hMutex) {
        CloseHandle(hFile);
        return;
    }

    HANDLE evNotEmpty = openEvent("QueueNotEmpty");
    HANDLE evNotFull = openEvent("QueueNotFull");

    signalSenderReady(senderId);

    handleSenderCommands(hFile, hMutex, evNotFull, evNotEmpty);

    vector<HANDLE> handles = { hFile, hMutex, evNotEmpty, evNotFull };
    cleanupHandles(handles);
}