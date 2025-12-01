#ifndef SYNC_UTILS_H
#define SYNC_UTILS_H

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// Вспомогательные функции
void printError(const string& context);
bool waitForObject(HANDLE handle, const string& context, DWORD timeout = 5000);
void cleanupHandles(const vector<HANDLE>& handles);

// Функции синхронизации
HANDLE createMutex();
HANDLE openMutex();
HANDLE createEvent(const string& name, bool initialState, bool manualReset = true);
HANDLE openEvent(const string& name);
vector<HANDLE> createReadyEvents(int nSenders);
void signalSenderReady(int senderId);

// Функции управления процессами
vector<PROCESS_INFORMATION> startAllSenders(const string& filename, int nSenders);
void waitForSendersReady(const vector<HANDLE>& readyEvents);
void terminateAllSenders(vector<PROCESS_INFORMATION>& processes);

#endif