#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <filesystem>
#include <map>
#include <conio.h>

namespace fs = std::filesystem;
using namespace std;

map<int, string> changedFiles;
int changeCount = 0;

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
                string filePath = directoryPath + "\\" + string(fileName.begin(), fileName.end());

                changeCount++;
                changedFiles[changeCount] = filePath;

                cout << "[" << changeCount << "] Change detected: " << filePath << endl;

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
    cout << "Press 'W' to stop monitoring." << endl;

    for (const string& drive : drives) {
        if (fs::exists(drive)) {
            threads.emplace_back(monitorDirectory, drive, ref(stopFlag));
        }
    }

    while (!stopFlag) {
        if (_kbhit()) {
            char key = _getch();
            if (key == 'W' || key == 'w') {
                stopFlag = true;
                break;
            }
        }
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    cout << "Monitoring stopped." << endl;

    if (!changedFiles.empty()) {
        cout << "Enter the number of the changed file to open its folder (or 0 to exit): ";
        int choice;
        cin >> choice;
        if (choice > 0 && changedFiles.count(choice)) {
            string filePath = changedFiles[choice];
            string command = "explorer /select,\"" + filePath + "\"";
            system(command.c_str());
        }
    }
}

int main() {
    int choice;
    bool stopFlag = false;

    cout << "Choose an option:" << endl;
    cout << "1. Manually specify a directory to monitor." << endl;
    cout << "2. Automatically monitor all drives for changes. (requires a powerful PC)" << endl;
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