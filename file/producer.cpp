#include <windows.h>
#include <iostream>
#include <chrono>

#define FILE_NAME L"shared_file.txt"
#define FILE_SIZE 100 * 1024 * 1024  // 100 MB
#define MUTEX_NAME L"FileIOMutex"

int main() {
    HANDLE hFile = CreateFileW(FILE_NAME, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create file: " << GetLastError() << "\n";
        return 1;
    }

    HANDLE hMutex = CreateMutexW(nullptr, FALSE, MUTEX_NAME);
    if (!hMutex) {
        std::cerr << "Could not create mutex: " << GetLastError() << "\n";
        CloseHandle(hFile);
        return 1;
    }

    char buffer[4096];
    memset(buffer, 'A', sizeof(buffer));

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < FILE_SIZE / sizeof(buffer); ++i) {
        DWORD bytesWritten;
        WriteFile(hFile, buffer, sizeof(buffer), &bytesWritten, nullptr);
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "File I/O Write Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";

    // Signal the reader by releasing the mutex
    ReleaseMutex(hMutex);

    CloseHandle(hFile);
    CloseHandle(hMutex);

    return 0;
}
