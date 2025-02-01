#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;
using namespace std;

void monitorDirectory(const string& directoryPath, bool& stopFlag) {
    wstring wideDirectoryPath = wstring(directoryPath.begin(), directoryPath.end());

    HANDLE hDir = CreateFile(
        wideDirectoryPath.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hDir == INVALID_HANDLE_VALUE) {
        cerr << "Failed to monitor directory: " << directoryPath << endl;
        return;
    }

    char buffer[1024];
    DWORD bytesReturned;

    while (!stopFlag) {
        if (ReadDirectoryChangesW(
            hDir,
            buffer,
            sizeof(buffer),
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned,
            NULL,
            NULL)) {

            FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)buffer;
            do {
                wstring fileName(fni->FileName, fni->FileNameLength / sizeof(WCHAR));
                cout << "Change detected in " << directoryPath << ": " << string(fileName.begin(), fileName.end()) << endl;

                fni = fni->NextEntryOffset ? (FILE_NOTIFY_INFORMATION*)((BYTE*)fni + fni->NextEntryOffset) : NULL;
            } while (fni);
        }
    }

    CloseHandle(hDir);
}

void monitorAllDrives(bool& stopFlag) {
    vector<string> drives = { "C:\\", "D:\\", "E:\\" };
    vector<thread> threads;

    cout << "Starting automatic monitoring of all drives..." << endl;

    for (const string& drive : drives) {
        if (fs::exists(drive)) {
            threads.emplace_back(monitorDirectory, drive, ref(stopFlag));
        }
    }

    cout << "Press Enter to stop monitoring." << endl;
    cin.get();
    stopFlag = true;

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    cout << "Monitoring stopped." << endl;
}

int main() {
    int choice;
    bool stopFlag = false;

    cout << "Choose an option:" << endl;
    cout << "1. Manually specify a directory to monitor." << endl;
    cout << "2. Automatically monitor all drives for changes. (requires a powerful pc)" << endl;
    cout << "Enter your choice (1 or 2): ";
    cin >> choice;
    cin.ignore();

    if (choice == 1) {
        string directoryPath;
        cout << "Enter the directory to monitor for changes: ";
        getline(cin, directoryPath);

        if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
            cerr << "The specified directory does not exist or is not valid." << endl;
            return 1;
        }

        cout << "Monitoring directory: " << directoryPath << endl;
        monitorDirectory(directoryPath, stopFlag);
    }
    else if (choice == 2) {
        monitorAllDrives(stopFlag);
    }
    else {
        cout << "Invalid choice. Exiting." << endl;
    }

    return 0;
}