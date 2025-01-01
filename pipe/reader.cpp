#include <windows.h>
#include <iostream>
#include <chrono>

#define PIPE_NAME L"\\\\.\\pipe\\MyBenchmarkPipe"
#define DATA_SIZE 1024 * 1024 * 10  // 100 MB
#define MUTEX_NAME L"PipeMutex"
#define SEM_READER_DONE_NAME L"PipeReaderDoneSemaphore"
#define SEM_WRITER_READY_NAME L"PipeWriterReadySemaphore"

int main() {
    // Open the named pipe
    HANDLE hPipe = CreateFileW(
        PIPE_NAME,
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open named pipe: " << GetLastError() << "\n";
        return 1;
    }

    // Open synchronization objects
    HANDLE hMutex = OpenMutexW(SYNCHRONIZE, FALSE, MUTEX_NAME);
    HANDLE hSemWriterReady = OpenSemaphoreW(SYNCHRONIZE, FALSE, SEM_WRITER_READY_NAME);
    HANDLE hSemReaderDone = OpenSemaphoreW(SEMAPHORE_MODIFY_STATE, FALSE, SEM_READER_DONE_NAME);

    if (!hMutex || !hSemWriterReady || !hSemReaderDone) {
        std::cerr << "Could not open synchronization objects: " << GetLastError() << "\n";
        CloseHandle(hPipe);
        return 1;
    }

    // Wait for the writer to signal readiness
    WaitForSingleObject(hSemWriterReady, INFINITE);

    // Read data from pipe
    WaitForSingleObject(hMutex, INFINITE);  // Lock mutex
    char* buffer = new char[DATA_SIZE];
    DWORD bytesRead;

    auto start = std::chrono::high_resolution_clock::now();
    ReadFile(hPipe, buffer, DATA_SIZE, &bytesRead, nullptr);
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Data read from pipe.\n";
    std::cout << "Time taken to read data: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n";

    ReleaseMutex(hMutex);  // Unlock mutex

    // Signal the writer that reading is done
    ReleaseSemaphore(hSemReaderDone, 1, nullptr);

    // Cleanup
    delete[] buffer;
    CloseHandle(hSemWriterReady);
    CloseHandle(hSemReaderDone);
    CloseHandle(hMutex);
    CloseHandle(hPipe);

    return 0;
}
