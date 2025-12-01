#include <windows.h>
#include <iostream>
#include <string>
#include "receiver.h"
#include "sender.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc == 1) {
        runReceiver();
    }
    else if (argc == 4 && string(argv[1]) == "sender") {
        string filename = argv[2];
        int id = stoi(argv[3]);
        runSender(filename, id);
    }
    else {
        cout << "Usage:\n"
            << "  OS_LAB_4.exe            - run Receiver\n"
            << "  OS_LAB_4.exe sender <file> <id> - run Sender\n";
    }

    return 0;
}