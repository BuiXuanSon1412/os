#include <windows.h>
#include <iostream>
#include <fstream>
#include <chrono>

#define FILENAME L"shared_memory.txt"
#define SHARED_MEMORY_NAME L"MySharedMemory"
#define DATA_SIZE 1024 * 1024 * 10  // 10 MB
#define MUTEX_NAME L"MySharedMemoryMutex"
#define SEM_READER_DONE_NAME L"ReaderDoneSemaphore"
#define SEM_WRITER_READY_NAME L"WriterReadySemaphore"
using namespace std;
using namespace chrono;
int main() {
    
    // Open synchronization objects
    HANDLE hMutex = OpenMutexW(SYNCHRONIZE, FALSE, MUTEX_NAME);
    HANDLE hSemWriterReady = OpenSemaphoreW(SEMAPHORE_MODIFY_STATE, FALSE, SEM_WRITER_READY_NAME);
    HANDLE hSemReaderDone = OpenSemaphoreW(SEMAPHORE_MODIFY_STATE, FALSE, SEM_READER_DONE_NAME);

    if (!hMutex || !hSemWriterReady || !hSemReaderDone) {
        std::cerr << "Could not open synchronization objects: " << GetLastError() << "\n";
        return 1;
    }

    // Wait for the writer to signal readiness
    WaitForSingleObject(hSemWriterReady, INFINITE);

    // Read data from shared memory
    WaitForSingleObject(hMutex, INFINITE);  // Lock mutex
    
    ifstream file(FILENAME, ios::binary);
    if (!file) {
        std::cerr << "Failed to open file for reading!" << std::endl;
        return 0;
    }

    // Create buffer to read data into
    char buffer[1];
    auto start = std::chrono::high_resolution_clock::now();

    // Read the entire file
    while (file.read(buffer, sizeof(buffer))) {
        // Process the data (in this case, we're just reading it)
    }

    file.close();
    
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Data read from shared memory.\n";
    std::cout << "Time taken to read data: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n";
    ReleaseMutex(hMutex);  // Unlock mutex

    // Signal the writer that reading is done
    if (ReleaseSemaphore(hSemReaderDone, 1, nullptr))
        std::cout << "Finish reading from reader" << std::endl;

    // Cleanup
    CloseHandle(hSemWriterReady);
    CloseHandle(hSemReaderDone);
    CloseHandle(hMutex);
    
    return 0;
}
