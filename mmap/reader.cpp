#include <windows.h>
#include <iostream>
#include <chrono>

#define SHARED_MEMORY_NAME L"MySharedMemory"
#define DATA_SIZE 1024 * 1024 * 100  // 10 MB
#define MUTEX_NAME L"MySharedMemoryMutex"
#define SEM_READER_DONE_NAME L"ReaderDoneSemaphore"
#define SEM_WRITER_READY_NAME L"WriterReadySemaphore"

int main() {
    // Open shared memory
    HANDLE hMapFile = OpenFileMappingW(FILE_MAP_READ, FALSE, SHARED_MEMORY_NAME);
    if (!hMapFile) {
        std::cerr << "Could not open shared memory: " << GetLastError() << "\n";
        return 1;
    }

    // Map shared memory
    char* pBuf = (char*)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, DATA_SIZE);
    if (!pBuf) {
        std::cerr << "Could not map shared memory: " << GetLastError() << "\n";
        CloseHandle(hMapFile);
        return 1;
    }

    // Open synchronization objects
    HANDLE hMutex = OpenMutexW(SYNCHRONIZE, FALSE, MUTEX_NAME);
    HANDLE hSemWriterReady = OpenSemaphoreW(SYNCHRONIZE, FALSE, SEM_WRITER_READY_NAME);
    HANDLE hSemReaderDone = OpenSemaphoreW(SYNCHRONIZE, FALSE, SEM_READER_DONE_NAME);

    if (!hMutex || !hSemWriterReady || !hSemReaderDone) {
        std::cerr << "Could not open synchronization objects: " << GetLastError() << "\n";
        UnmapViewOfFile(pBuf);
        CloseHandle(hMapFile);
        return 1;
    }

    // Wait for the writer to signal readiness
    WaitForSingleObject(hSemWriterReady, INFINITE);

    // Read data from shared memory
    WaitForSingleObject(hMutex, INFINITE);  // Lock mutex
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < DATA_SIZE; ++i) {
        volatile char temp = pBuf[i];  // Simulate reading
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Data read from shared memory.\n";
    std::cout << "Time taken to read data: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n";
    ReleaseMutex(hMutex);  // Unlock mutex

    // Signal the writer that reading is done
    ReleaseSemaphore(hSemReaderDone, 1, nullptr);

    // Cleanup
    CloseHandle(hSemWriterReady);
    CloseHandle(hSemReaderDone);
    CloseHandle(hMutex);
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);

    return 0;
}
