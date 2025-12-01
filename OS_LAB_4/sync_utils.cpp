#include "sync_utils.h"

void printError(const string& context) {
    DWORD error = GetLastError();
    cout << context << " Error code: " << error << "\n";
}

bool waitForObject(HANDLE handle, const string& context, DWORD timeout) {
    DWORD waitResult = WaitForSingleObject(handle, timeout);
    if (waitResult != WAIT_OBJECT_0) {
        cout << context << " timeout or error\n";
        return false;
    }
    return true;
}

void cleanupHandles(const vector<HANDLE>& handles) {
    for (HANDLE handle : handles) {
        if (handle) {
            CloseHandle(handle);
        }
    }
}

HANDLE createMutex() {
    HANDLE hMutex = CreateMutexA(NULL, FALSE, "QueueMutex");
    if (!hMutex) {
        printError("Failed to create mutex");
    }
    return hMutex;
}

HANDLE openMutex() {
    HANDLE hMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, "QueueMutex");
    if (!hMutex) {
        printError("Failed to open mutex");
    }
    return hMutex;
}

HANDLE createEvent(const string& name, bool initialState, bool manualReset) {
    HANDLE hEvent = CreateEventA(NULL, manualReset, initialState, name.c_str());
    if (!hEvent) {
        printError("Failed to create event: " + name);
    }
    return hEvent;
}

HANDLE openEvent(const string& name) {
    HANDLE hEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, name.c_str());
    if (!hEvent) {
        printError("Failed to open event: " + name);
    }
    return hEvent;
}

vector<HANDLE> createReadyEvents(int nSenders) {
    vector<HANDLE> readyEvents;

    for (int i = 0; i < nSenders; ++i) {
        string evName = "SenderReady_" + to_string(i);
        HANDLE ev = createEvent(evName, false);
        readyEvents.push_back(ev);
    }

    return readyEvents;
}

void signalSenderReady(int senderId) {
    string evName = "SenderReady_" + to_string(senderId);
    HANDLE evReady = openEvent(evName);

    if (evReady) {
        SetEvent(evReady);
        cout << "Sender #" << senderId << " ready.\n";
        CloseHandle(evReady);
    }
}

vector<PROCESS_INFORMATION> startAllSenders(const string& filename, int nSenders) {
    vector<PROCESS_INFORMATION> processes;

    for (int i = 0; i < nSenders; ++i) {
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);

        string cmd = string(exePath) + " sender " + filename + " " + to_string(i);

        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        BOOL success = CreateProcessA(
            NULL,
            (LPSTR)cmd.c_str(),
            NULL, NULL,
            FALSE,
            CREATE_NEW_CONSOLE,
            NULL, NULL,
            &si, &pi
        );

        if (success) {
            processes.push_back(pi);
            cout << "Started sender #" << i << " with PID: " << pi.dwProcessId << "\n";
        }
        else {
            printError("Failed to start sender #" + to_string(i));
            cout << "Command was: " << cmd << "\n";
        }
    }

    return processes;
}

void waitForSendersReady(const vector<HANDLE>& readyEvents) {
    cout << "Waiting for senders to be ready...\n";

    DWORD waitResult = WaitForMultipleObjects(
        readyEvents.size(), readyEvents.data(), TRUE, 10000);

    if (waitResult == WAIT_TIMEOUT) {
        cout << "Timeout waiting for senders. Some senders may not be ready.\n";
        cout << "You can manually start senders with command:\n";

        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);

        for (size_t i = 0; i < readyEvents.size(); ++i) {
            cout << exePath << " sender <filename> " << i << "\n";
        }
    }
    else if (waitResult == WAIT_FAILED) {
        printError("Error waiting for senders");
    }
    else {
        cout << "All senders are ready.\n";
    }
}

void terminateAllSenders(vector<PROCESS_INFORMATION>& processes) {
    for (auto& pi : processes) {
        TerminateProcess(pi.hProcess, 0);
        WaitForSingleObject(pi.hProcess, 1000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}