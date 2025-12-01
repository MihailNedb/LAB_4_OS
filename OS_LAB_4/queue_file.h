#ifndef QUEUE_FILE_H
#define QUEUE_FILE_H

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// Структура заголовка очереди
#pragma pack(push,1)
struct QueueHeader {
    int capacity;       // максимальное число сообщений
    int head;           // позиция чтения
    int tail;           // позиция записи
    int count;          // текущее количество сообщений
};
#pragma pack(pop)

// Константы
const int MSG_SIZE = 20;

// Функции работы с файлом очереди
HANDLE openFile(const string& filename, bool createNew = false);
bool initializeQueueFile(HANDLE hFile, int capacity);
bool readQueueHeader(HANDLE hFile, QueueHeader& header);
bool writeQueueHeader(HANDLE hFile, const QueueHeader& header);
bool readMessage(HANDLE hFile, const QueueHeader& header, int index, char* buffer);
bool writeMessage(HANDLE hFile, const QueueHeader& header, int index, const string& message);

#endif