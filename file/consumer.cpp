#include <windows.h>
#include <iostream>
#include <chrono>

#define FILE_NAME L"shared_file.txt"
#define FILE_SIZE 100 * 1024 * 1024  // 100 MB
#define MUTEX_NAME L"FileIOMutex"

int main() {
    HANDLE hMutex = OpenMutexW(SYNCHRONIZE, FALSE, MUTEX_NAME);
    if (!hMutex) {
        std::cerr << "Could not open mutex: " << GetLastError() << "\n";
        return 1;
    }

    std::cout << "Waiting for writer...\n";

    // Wait until writer releases the mutex
    WaitForSingleObject(hMutex, INFINITE);

    HANDLE hFile = CreateFileW(FILE_NAME, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open file: " << GetLastError() << "\n";
        return 1;
    }

    char buffer[4096];
    size_t bytesReadTotal = 0;

    auto start = std::chrono::high_resolution_clock::now();
    while (ReadFile(hFile, buffer, sizeof(buffer), nullptr, nullptr)) {
        bytesReadTotal += sizeof(buffer);
        if (bytesReadTotal >= FILE_SIZE) break;
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "File I/O Read Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";

    CloseHandle(hFile);
    CloseHandle(hMutex);

    return 0;
}
