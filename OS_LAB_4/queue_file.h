#ifndef QUEUE_FILE_H
#define QUEUE_FILE_H

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

#pragma pack(push,1)
struct QueueHeader {
    int capacity;       
    int head;           
    int tail;           
    int count;          
};
#pragma pack(pop)

const int MSG_SIZE = 20;

HANDLE openFile(const string& filename, bool createNew = false);
bool initializeQueueFile(HANDLE hFile, int capacity);
bool readQueueHeader(HANDLE hFile, QueueHeader& header);
bool writeQueueHeader(HANDLE hFile, const QueueHeader& header);
bool readMessage(HANDLE hFile, const QueueHeader& header, int index, char* buffer);
bool writeMessage(HANDLE hFile, const QueueHeader& header, int index, const string& message);

#endif