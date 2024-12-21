#include <windows.h>
#include <iostream>
#include <chrono>

#define SHARED_MEMORY_NAME L"MySharedMemory"
#define DATA_SIZE 1024 * 1024 * 100  // 10 MB
#define MUTEX_NAME L"MySharedMemoryMutex"
#define SEM_READER_DONE_NAME L"ReaderDoneSemaphore"
#define SEM_WRITER_READY_NAME L"WriterReadySemaphore"

int main() {
    // Create shared memory
    HANDLE hMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, DATA_SIZE, SHARED_MEMORY_NAME);
    if (!hMapFile) {
        std::cerr << "Could not create shared memory: " << GetLastError() << "\n";
        return 1;
    }

    // Map shared memory
    char* pBuf = (char*)MapViewOfFile(hMapFile, FILE_MAP_WRITE, 0, 0, DATA_SIZE);
    if (!pBuf) {
        std::cerr << "Could not map shared memory: " << GetLastError() << "\n";
        CloseHandle(hMapFile);
        return 1;
    }

    // Create synchronization objects
    HANDLE hMutex = CreateMutexW(nullptr, FALSE, MUTEX_NAME);
    HANDLE hSemWriterReady = CreateSemaphoreW(nullptr, 0, 1, SEM_WRITER_READY_NAME);
    HANDLE hSemReaderDone = CreateSemaphoreW(nullptr, 0, 1, SEM_READER_DONE_NAME);

    if (!hMutex || !hSemWriterReady || !hSemReaderDone) {
        std::cerr << "Could not create synchronization objects: " << GetLastError() << "\n";
        UnmapViewOfFile(pBuf);
        CloseHandle(hMapFile);
        return 1;
    }

    // Write data to shared memory
    WaitForSingleObject(hMutex, INFINITE);  // Lock mutex
    auto start = std::chrono::high_resolution_clock::now();
    memset(pBuf, 'A', DATA_SIZE);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Data written to shared memory.\n";
    std::cout << "Time taken to write data: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n";
    ReleaseMutex(hMutex);  // Unlock mutex

    // Signal the reader that data is ready
    ReleaseSemaphore(hSemWriterReady, 1, nullptr);

    // Wait for the reader to signal completion
    std::cout << "Waiting for reader to finish...\n";
    WaitForSingleObject(hSemReaderDone, INFINITE);
    std::cout << "Reader finished. Exiting writer program.\n";

    // Cleanup
    CloseHandle(hSemWriterReady);
    CloseHandle(hSemReaderDone);
    CloseHandle(hMutex);
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);

    return 0;
}
