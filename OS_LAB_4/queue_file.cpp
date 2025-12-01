#include "queue_file.h"

HANDLE openFile(const string& filename, bool createNew) {
    DWORD access = GENERIC_READ | GENERIC_WRITE;
    DWORD creation = createNew ? CREATE_ALWAYS : OPEN_EXISTING;

    HANDLE hFile = CreateFileA(filename.c_str(), access,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        cout << "Cannot open file: " << filename << " Error code: " << error << "\n";
    }

    return hFile;
}

bool initializeQueueFile(HANDLE hFile, int capacity) {
    QueueHeader q = { capacity, 0, 0, 0 };
    DWORD rw;

    if (!WriteFile(hFile, &q, sizeof(q), &rw, NULL)) {
        DWORD error = GetLastError();
        cout << "Failed to write queue header. Error code: " << error << "\n";
        return false;
    }

    vector<char> zeros(MSG_SIZE * capacity, 0);
    if (!WriteFile(hFile, zeros.data(), zeros.size(), &rw, NULL)) {
        DWORD error = GetLastError();
        cout << "Failed to initialize message storage. Error code: " << error << "\n";
        return false;
    }

    return true;
}

bool readQueueHeader(HANDLE hFile, QueueHeader& header) {
    DWORD rw;
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    if (!ReadFile(hFile, &header, sizeof(header), &rw, NULL)) {
        DWORD error = GetLastError();
        cout << "Failed to read queue header. Error code: " << error << "\n";
        return false;
    }

    return true;
}

bool writeQueueHeader(HANDLE hFile, const QueueHeader& header) {
    DWORD rw;
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    if (!WriteFile(hFile, &header, sizeof(header), &rw, NULL)) {
        DWORD error = GetLastError();
        cout << "Failed to write queue header. Error code: " << error << "\n";
        return false;
    }

    return true;
}

bool readMessage(HANDLE hFile, const QueueHeader& header, int index, char* buffer) {
    DWORD rw;
    SetFilePointer(hFile, sizeof(header) + index * MSG_SIZE, NULL, FILE_BEGIN);

    if (!ReadFile(hFile, buffer, MSG_SIZE, &rw, NULL)) {
        DWORD error = GetLastError();
        cout << "Failed to read message. Error code: " << error << "\n";
        return false;
    }

    buffer[MSG_SIZE] = '\0'; // Ensure null termination
    return true;
}

bool writeMessage(HANDLE hFile, const QueueHeader& header, int index, const string& message) {
    DWORD rw;
    SetFilePointer(hFile, sizeof(header) + index * MSG_SIZE, NULL, FILE_BEGIN);

    char buf[MSG_SIZE] = { 0 };
    memcpy(buf, message.c_str(), min(message.size(), (size_t)MSG_SIZE));

    if (!WriteFile(hFile, buf, MSG_SIZE, &rw, NULL)) {
        DWORD error = GetLastError();
        cout << "Failed to write message. Error code: " << error << "\n";
        return false;
    }

    return true;
}