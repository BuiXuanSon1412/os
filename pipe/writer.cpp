#include <windows.h>
#include <iostream>
#include <chrono>

#define PIPE_NAME L"\\\\.\\pipe\\MyBenchmarkPipe"
#define DATA_SIZE 1024 * 1024 * 100  // 10 MB
#define MUTEX_NAME L"PipeMutex"
#define SEM_READER_DONE_NAME L"PipeReaderDoneSemaphore"
#define SEM_WRITER_READY_NAME L"PipeWriterReadySemaphore"

int main() {
    // Create named pipe
    HANDLE hPipe = CreateNamedPipeW(
        PIPE_NAME,
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        DATA_SIZE,
        0,
        0,
        nullptr);

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create named pipe: " << GetLastError() << "\n";
        return 1;
    }

    // Create synchronization objects
    HANDLE hMutex = CreateMutexW(nullptr, FALSE, MUTEX_NAME);
    HANDLE hSemWriterReady = CreateSemaphoreW(nullptr, 0, 1, SEM_WRITER_READY_NAME);
    HANDLE hSemReaderDone = CreateSemaphoreW(nullptr, 0, 1, SEM_READER_DONE_NAME);

    if (!hMutex || !hSemWriterReady || !hSemReaderDone) {
        std::cerr << "Could not create synchronization objects: " << GetLastError() << "\n";
        CloseHandle(hPipe);
        return 1;
    }

    // Wait for the reader to connect
    std::cout << "Waiting for reader to connect...\n";
    ConnectNamedPipe(hPipe, nullptr);

    // Write data to pipe
    WaitForSingleObject(hMutex, INFINITE);  // Lock mutex
    char* buffer = new char[DATA_SIZE];
    memset(buffer, 'A', DATA_SIZE);

    auto start = std::chrono::high_resolution_clock::now();
    DWORD bytesWritten;
    WriteFile(hPipe, buffer, DATA_SIZE, &bytesWritten, nullptr);
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Data written to pipe.\n";
    std::cout << "Time taken to write data: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n";

    ReleaseMutex(hMutex);  // Unlock mutex

    // Signal that writing is complete
    ReleaseSemaphore(hSemWriterReady, 1, nullptr);

    // Wait for the reader to signal completion
    std::cout << "Waiting for reader to finish...\n";
    WaitForSingleObject(hSemReaderDone, INFINITE);
    std::cout << "Reader finished. Exiting writer program.\n";

    // Cleanup
    delete[] buffer;
    CloseHandle(hSemWriterReady);
    CloseHandle(hSemReaderDone);
    CloseHandle(hMutex);
    CloseHandle(hPipe);

    return 0;
}
