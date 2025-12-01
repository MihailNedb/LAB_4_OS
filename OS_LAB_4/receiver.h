#ifndef RECEIVER_H
#define RECEIVER_H

#include <windows.h>
#include <iostream>
#include <string>
#include "queue_file.h"
#include "sync_utils.h"

using namespace std;

// Основные функции receiver
void runReceiver();
void handleReceiverCommands(HANDLE hFile, HANDLE hMutex, HANDLE evNotEmpty, HANDLE evNotFull);
void processReadCommand(HANDLE hFile, HANDLE hMutex, HANDLE evNotEmpty, HANDLE evNotFull);

#endif