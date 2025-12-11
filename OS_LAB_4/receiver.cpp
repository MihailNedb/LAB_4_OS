#include "receiver.h"

void processReadCommand(HANDLE hFile, HANDLE hMutex, HANDLE evNotEmpty, HANDLE evNotFull) {
 
    if (!waitForObject(evNotEmpty, "Waiting for messages")) {
        return;
    }

  
    if (!waitForObject(hMutex, "Waiting for mutex")) {
        return;
    }

  
    QueueHeader h;
    if (!readQueueHeader(hFile, h)) {
        ReleaseMutex(hMutex);
        return;
    }

    int index = h.head;
    h.head = (h.head + 1) % h.capacity;
    h.count--;

    char buf[MSG_SIZE + 1] = { 0 };
    if (!readMessage(hFile, h, index, buf)) {
        ReleaseMutex(hMutex);
        return;
    }

    if (!writeQueueHeader(hFile, h)) {
        ReleaseMutex(hMutex);
        return;
    }

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

    HANDLE hFile = openFile(filename, true);
    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }

    if (!initializeQueueFile(hFile, capacity)) {
        CloseHandle(hFile);
        return;
    }

    HANDLE hMutex = createMutex();
    HANDLE evNotEmpty = createEvent("QueueNotEmpty", false);
    HANDLE evNotFull = createEvent("QueueNotFull", true);

    if (!hMutex || !evNotEmpty || !evNotFull) {
        cleanupHandles({ hFile, hMutex, evNotEmpty, evNotFull });
        return;
    }

    int nSenders;
    cout << "Number of senders: ";
    cin >> nSenders;

    vector<HANDLE> readyEvents = createReadyEvents(nSenders);

 
    vector<PROCESS_INFORMATION> processes = startAllSenders(filename, nSenders);

    
    waitForSendersReady(readyEvents);


    handleReceiverCommands(hFile, hMutex, evNotEmpty, evNotFull);

   
    terminateAllSenders(processes);

    cleanupHandles(readyEvents);
    cleanupHandles({ hFile, hMutex, evNotEmpty, evNotFull });
}