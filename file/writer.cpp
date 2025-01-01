#include <windows.h>
#include <iostream>
#include <chrono>
#include <fstream>

#define FILENAME L"shared_memory.txt"
#define DATA_SIZE 1024 * 1024 * 10  // 10 MB
#define MUTEX_NAME L"MySharedMemoryMutex"
#define SEM_READER_DONE_NAME L"ReaderDoneSemaphore"
#define SEM_WRITER_READY_NAME L"WriterReadySemaphore"

using namespace std;
using namespace chrono;

int main() {

    // Create synchronization objects
    HANDLE hMutex = CreateMutexW(nullptr, FALSE, MUTEX_NAME);
    HANDLE hSemWriterReady = CreateSemaphoreW(nullptr, 0, 1, SEM_WRITER_READY_NAME);
    HANDLE hSemReaderDone = CreateSemaphoreW(nullptr, 0, 1, SEM_READER_DONE_NAME);

    if (!hMutex || !hSemWriterReady || !hSemReaderDone) {
        std::cerr << "Could not create synchronization objects: " << GetLastError() << "\n";
        return 1;
    }

    // Write data to shared memory
    WaitForSingleObject(hMutex, INFINITE);  // Lock mutex
    
    std::ofstream file(FILENAME, ios::binary);
    if (!file) {
        std::cerr << "Failed to open file for writing!" << std::endl;
        return 0;
    }

    // Create dummy data to write
    char buffer[] = "A";
    //fill(begin(buffer), end(buffer), 'A');

    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < DATA_SIZE; ++i) {
        file.write(buffer, sizeof(buffer));
    }

    file.close();
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Data written to shared memory.\n";
    std::cout << "Time taken to write data: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n";
    ReleaseMutex(hMutex);  // Unlock mutex

    // Signal the reader that data is ready
    if (ReleaseSemaphore(hSemWriterReady, 1, nullptr))
        std::cout << "Release for reader to read" << std::endl;

    // Wait for the reader to signal completion
    std::cout << "Waiting for reader to finish...\n";
    WaitForSingleObject(hSemReaderDone, INFINITE);  // Wait for reader to finish
    std::cout << "Reader finished. Exiting writer program.\n";

    // Cleanup
    CloseHandle(hSemWriterReady);
    CloseHandle(hSemReaderDone);
    CloseHandle(hMutex);
    
    return 0;
}
