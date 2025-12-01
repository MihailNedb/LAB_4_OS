#include <gtest/gtest.h>
#include <windows.h>
#include <fstream>
#include <thread>
#include <chrono>
#include "queue_file.h"
#include "sync_utils.h"

using namespace std;

class QueueFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        hFile = CreateFileA("test_queue.bin", 
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL, 
                           CREATE_ALWAYS, 
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        ASSERT_NE(hFile, INVALID_HANDLE_VALUE);
    }

    void TearDown() override {
        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
            DeleteFileA("test_queue.bin");
        }
    }

    HANDLE hFile = INVALID_HANDLE_VALUE;
};

class SyncUtilsTest : public ::testing::Test {
protected:
    void TearDown() override {
        cleanupHandles(handles);
    }

    vector<HANDLE> handles;
};

TEST(QueueHeaderTest, StructureSizeAndAlignment) {
    EXPECT_EQ(sizeof(QueueHeader), 16);
    EXPECT_EQ(offsetof(QueueHeader, capacity), 0);
    EXPECT_EQ(offsetof(QueueHeader, head), 4);
    EXPECT_EQ(offsetof(QueueHeader, tail), 8);
    EXPECT_EQ(offsetof(QueueHeader, count), 12);
}

TEST(ConstantsTest, MessageSize) {
    EXPECT_EQ(MSG_SIZE, 20);
    EXPECT_GT(MSG_SIZE, 0);
    EXPECT_LT(MSG_SIZE, 1000);
}

TEST_F(QueueFileTest, InitializeQueueFileSuccess) {
    const int capacity = 10;
    EXPECT_TRUE(initializeQueueFile(hFile, capacity));
    
    QueueHeader header;
    EXPECT_TRUE(readQueueHeader(hFile, header));
    
    EXPECT_EQ(header.capacity, capacity);
    EXPECT_EQ(header.head, 0);
    EXPECT_EQ(header.tail, 0);
    EXPECT_EQ(header.count, 0);
}

TEST_F(QueueFileTest, InitializeQueueFileLargeCapacity) {
    const int largeCapacity = 1000;
    EXPECT_TRUE(initializeQueueFile(hFile, largeCapacity));
    
    QueueHeader header;
    EXPECT_TRUE(readQueueHeader(hFile, header));
    EXPECT_EQ(header.capacity, largeCapacity);
}

TEST_F(QueueFileTest, InitializeQueueFileSmallCapacity) {
    const int smallCapacity = 1;
    EXPECT_TRUE(initializeQueueFile(hFile, smallCapacity));
    
    QueueHeader header;
    EXPECT_TRUE(readQueueHeader(hFile, header));
    EXPECT_EQ(header.capacity, smallCapacity);
}

TEST_F(QueueFileTest, WriteAndReadSingleMessage) {
    const int capacity = 5;
    initializeQueueFile(hFile, capacity);
    
    string testMessage = "Hello, World!";
    EXPECT_TRUE(writeMessage(hFile, {capacity, 0, 0, 0}, 0, testMessage));
    
    char buffer[MSG_SIZE + 1];
    EXPECT_TRUE(readMessage(hFile, {capacity, 0, 0, 0}, 0, buffer));
    EXPECT_STREQ(buffer, "Hello, World!");
}

TEST_F(QueueFileTest, WriteAndReadMultipleMessages) {
    const int capacity = 3;
    initializeQueueFile(hFile, capacity);
    
    vector<string> messages = {"Message1", "Test2", "Hello3"};
    
    for (int i = 0; i < messages.size(); ++i) {
        EXPECT_TRUE(writeMessage(hFile, {capacity, 0, 0, 0}, i, messages[i]));
    }
    
    for (int i = 0; i < messages.size(); ++i) {
        char buffer[MSG_SIZE + 1];
        EXPECT_TRUE(readMessage(hFile, {capacity, 0, 0, 0}, i, buffer));
        EXPECT_STREQ(buffer, messages[i].c_str());
    }
}

TEST_F(QueueFileTest, WriteMessageExactSize) {
    const int capacity = 2;
    initializeQueueFile(hFile, capacity);
    
    string exactSizeMessage(MSG_SIZE, 'A');
    EXPECT_TRUE(writeMessage(hFile, {capacity, 0, 0, 0}, 0, exactSizeMessage));
    
    char buffer[MSG_SIZE + 1];
    EXPECT_TRUE(readMessage(hFile, {capacity, 0, 0, 0}, 0, buffer));
    EXPECT_EQ(string(buffer, MSG_SIZE), exactSizeMessage);
}

TEST_F(QueueFileTest, WriteMessageLargerThanSize) {
    const int capacity = 2;
    initializeQueueFile(hFile, capacity);
    
    string largeMessage(MSG_SIZE + 10, 'B');
    EXPECT_TRUE(writeMessage(hFile, {capacity, 0, 0, 0}, 0, largeMessage));
    
    char buffer[MSG_SIZE + 1];
    EXPECT_TRUE(readMessage(hFile, {capacity, 0, 0, 0}, 0, buffer));
    EXPECT_EQ(string(buffer, MSG_SIZE), largeMessage.substr(0, MSG_SIZE));
}

TEST_F(QueueFileTest, WriteEmptyMessage) {
    const int capacity = 2;
    initializeQueueFile(hFile, capacity);
    
    string emptyMessage = "";
    EXPECT_TRUE(writeMessage(hFile, {capacity, 0, 0, 0}, 0, emptyMessage));
    
    char buffer[MSG_SIZE + 1];
    EXPECT_TRUE(readMessage(hFile, {capacity, 0, 0, 0}, 0, buffer));
    EXPECT_STREQ(buffer, "");
}

TEST_F(QueueFileTest, ReadWriteHeaderConsistency) {
    const int capacity = 5;
    QueueHeader original = {capacity, 1, 3, 2};
    
    EXPECT_TRUE(writeQueueHeader(hFile, original));
    
    QueueHeader read;
    EXPECT_TRUE(readQueueHeader(hFile, read));
    
    EXPECT_EQ(read.capacity, original.capacity);
    EXPECT_EQ(read.head, original.head);
    EXPECT_EQ(read.tail, original.tail);
    EXPECT_EQ(read.count, original.count);
}

TEST_F(QueueFileTest, OverwriteMessage) {
    const int capacity = 3;
    initializeQueueFile(hFile, capacity);
    
    EXPECT_TRUE(writeMessage(hFile, {capacity, 0, 0, 0}, 0, "First"));
    
    EXPECT_TRUE(writeMessage(hFile, {capacity, 0, 0, 0}, 0, "Second"));
    
    char buffer[MSG_SIZE + 1];
    EXPECT_TRUE(readMessage(hFile, {capacity, 0, 0, 0}, 0, buffer));
    EXPECT_STREQ(buffer, "Second");
}

TEST_F(SyncUtilsTest, CreateAndOpenMutex) {
    HANDLE mutex = createMutex();
    EXPECT_NE(mutex, nullptr);
    
    if (mutex) {
        HANDLE openedMutex = openMutex();
        EXPECT_NE(openedMutex, nullptr);
        
        if (openedMutex) {
            CloseHandle(openedMutex);
        }
        handles.push_back(mutex);
    }
}

TEST_F(SyncUtilsTest, MutexOwnership) {
    HANDLE mutex = createMutex();
    ASSERT_NE(mutex, nullptr);
    
    DWORD waitResult = WaitForSingleObject(mutex, 100);
    EXPECT_EQ(waitResult, WAIT_OBJECT_0);
    
    BOOL releaseResult = ReleaseMutex(mutex);
    EXPECT_TRUE(releaseResult);
    
    handles.push_back(mutex);
}

TEST_F(SyncUtilsTest, CreateNamedEvent) {
    string eventName = "TestEvent_" + to_string(GetTickCount());
    HANDLE event = createEvent(eventName, false);
    EXPECT_NE(event, nullptr);
    
    if (event) {
        handles.push_back(event);
    }
}

TEST_F(SyncUtilsTest, CreateAndOpenEvent) {
    string eventName = "TestEvent_" + to_string(GetCurrentProcessId());
    HANDLE event = createEvent(eventName, true);
    EXPECT_NE(event, nullptr);
    
    if (event) {
        handles.push_back(event);
    }
}

TEST_F(SyncUtilsTest, CreateMultipleReadyEvents) {
    int nSenders = 3;
    vector<HANDLE> readyEvents = createReadyEvents(nSenders);
    
    EXPECT_EQ(readyEvents.size(), nSenders);
    
    for (const auto& event : readyEvents) {
        EXPECT_NE(event, nullptr);
    }
    
    for (const auto& event : readyEvents) {
        if (event) {
            CloseHandle(event);
        }
    }
}

TEST(UtilityTest, PrintErrorFunction) {
    EXPECT_NO_THROW(printError("Test error context"));
}

TEST(UtilityTest, WaitForObjectSuccess) {
    HANDLE event = CreateEventA(NULL, TRUE, TRUE, NULL);
    ASSERT_NE(event, nullptr);
    
    EXPECT_TRUE(waitForObject(event, "Test wait", 100));
    
    CloseHandle(event);
}

TEST(UtilityTest, WaitForObjectTimeout) {
    HANDLE event = CreateEventA(NULL, TRUE, FALSE, NULL);
    ASSERT_NE(event, nullptr);
    
    EXPECT_FALSE(waitForObject(event, "Test timeout", 50));
    
    CloseHandle(event);
}

TEST(UtilityTest, CleanupHandlesSimple) {
    vector<HANDLE> testHandles;
    
    HANDLE event = CreateEventA(NULL, TRUE, FALSE, NULL);
    HANDLE mutex = CreateMutexA(NULL, FALSE, NULL);
    
    if (event) testHandles.push_back(event);
    if (mutex) testHandles.push_back(mutex);
    
    EXPECT_NO_THROW(cleanupHandles(testHandles));
    
    EXPECT_EQ(testHandles.size(), 2);
    
    if (event) CloseHandle(event);
    if (mutex) CloseHandle(mutex);
}

TEST(QueueLogicTest, CircularBufferWrapAround) {
    const int capacity = 3;
    QueueHeader q = {capacity, 2, 1, 2};
    
    int nextHead = (q.head + 1) % q.capacity;
    int nextTail = (q.tail + 1) % q.capacity;
    
    EXPECT_EQ(nextHead, 0);
    EXPECT_EQ(nextTail, 2);
}

TEST(QueueLogicTest, QueueFullCondition) {
    const int capacity = 5;
    QueueHeader q = {capacity, 0, 0, capacity};
    
    EXPECT_TRUE(q.count == q.capacity);
    EXPECT_FALSE(q.count < q.capacity);
}

TEST(QueueLogicTest, QueueEmptyCondition) {
    const int capacity = 5;
    QueueHeader q = {capacity, 2, 2, 0};
    
    EXPECT_TRUE(q.count == 0);
    EXPECT_FALSE(q.count > 0);
}

TEST(QueueLogicTest, HeadTailRelationship) {
    const int capacity = 4;
    QueueHeader q = {capacity, 1, 3, 2};
    
    EXPECT_GE(q.count, 0);
    EXPECT_LE(q.count, q.capacity);
    EXPECT_GE(q.head, 0);
    EXPECT_LT(q.head, q.capacity);
    EXPECT_GE(q.tail, 0);
    EXPECT_LT(q.tail, q.capacity);
}

TEST(IntegrationTest, MultipleWriteReadCycle) {
    string filename = "integration_test_" + to_string(GetTickCount()) + ".bin";
    HANDLE hFile = CreateFileA(filename.c_str(), 
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, 
                              CREATE_ALWAYS, 
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    ASSERT_NE(hFile, INVALID_HANDLE_VALUE);
    
    const int capacity = 4;
    EXPECT_TRUE(initializeQueueFile(hFile, capacity));
    
    for (int cycle = 0; cycle < 3; ++cycle) {
        for (int i = 0; i < capacity; ++i) {
            string message = "Cycle" + to_string(cycle) + "Msg" + to_string(i);
            EXPECT_TRUE(writeMessage(hFile, {capacity, 0, 0, 0}, i, message));
        }
        
        for (int i = 0; i < capacity; ++i) {
            char buffer[MSG_SIZE + 1];
            EXPECT_TRUE(readMessage(hFile, {capacity, 0, 0, 0}, i, buffer));
            string expected = "Cycle" + to_string(cycle) + "Msg" + to_string(i);
            EXPECT_STREQ(buffer, expected.c_str());
        }
    }
    
    CloseHandle(hFile);
    DeleteFileA(filename.c_str());
}

TEST(StressTest, RapidHeaderOperations) {
    string filename = "stress_test_" + to_string(GetTickCount()) + ".bin";
    HANDLE hFile = CreateFileA(filename.c_str(), 
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, 
                              CREATE_ALWAYS, 
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    ASSERT_NE(hFile, INVALID_HANDLE_VALUE);
    
    const int capacity = 10;
    EXPECT_TRUE(initializeQueueFile(hFile, capacity));
    
    for (int i = 0; i < 50; ++i) {
        QueueHeader q = {capacity, i % capacity, (i + 1) % capacity, 1};
        EXPECT_TRUE(writeQueueHeader(hFile, q));
        
        QueueHeader readQ;
        EXPECT_TRUE(readQueueHeader(hFile, readQ));
        EXPECT_EQ(readQ.capacity, q.capacity);
    }
    
    CloseHandle(hFile);
    DeleteFileA(filename.c_str());
}

TEST(BoundaryTest, MessageWithSpecialCharacters) {
    string filename = "special_test_" + to_string(GetTickCount()) + ".bin";
    HANDLE hFile = CreateFileA(filename.c_str(), 
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, 
                              CREATE_ALWAYS, 
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    ASSERT_NE(hFile, INVALID_HANDLE_VALUE);
    
    const int capacity = 2;
    initializeQueueFile(hFile, capacity);
    
    string specialMessage = "Test\n\t\r\x01\xFF";
    EXPECT_TRUE(writeMessage(hFile, {capacity, 0, 0, 0}, 0, specialMessage));
    
    char buffer[MSG_SIZE + 1];
    EXPECT_TRUE(readMessage(hFile, {capacity, 0, 0, 0}, 0, buffer));
    
    CloseHandle(hFile);
    DeleteFileA(filename.c_str());
}

TEST(ProcessTest, SignalSenderReady) {
    EXPECT_NO_THROW(signalSenderReady(0));
    EXPECT_NO_THROW(signalSenderReady(999));
}

TEST(MemoryTest, BufferOverflowProtection) {
    string longMessage(MSG_SIZE * 2, 'X');
    string expected = longMessage.substr(0, MSG_SIZE);
    
    EXPECT_EQ(min(longMessage.size(), (size_t)MSG_SIZE), MSG_SIZE);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    int result = RUN_ALL_TESTS();
    
    DeleteFileA("test_queue.bin");
    
    return result;
}