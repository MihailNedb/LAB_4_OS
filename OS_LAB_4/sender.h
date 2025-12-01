#ifndef SENDER_H
#define SENDER_H

#include <windows.h>
#include <iostream>
#include <string>
#include "queue_file.h"
#include "sync_utils.h"

using namespace std;

// Основные функции sender
void runSender(string filename, int senderId);
void handleSenderCommands(HANDLE hFile, HANDLE hMutex, HANDLE evNotFull, HANDLE evNotEmpty);
void processSendCommand(HANDLE hFile, HANDLE hMutex, HANDLE evNotFull, HANDLE evNotEmpty);

#endif