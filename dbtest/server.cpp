#include <windows.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <sstream>
#include <shared_mutex>
#include <mutex>
#include <chrono>
#include <thread>

// Constants for memory-mapped file and database file
const wchar_t* MEMORY_MAPPED_FILE_NAME = L"MyMappedFile";
const char* DATABASE_FILE = "database_mmap.txt";
const size_t MAPPED_FILE_SIZE = 1024 * 1024; // 1 MB

// Shared data structure for communication via memory-mapped file
struct SharedData {
    char request[512];
    char response[512];
    bool requestReady;
    bool responseReady;
};

// Mutex for synchronization (read-write lock)
std::shared_mutex dbMutex;  // Allows multiple readers, exclusive writer

// In-memory cache (for the database)
std::unordered_map<std::string, std::string> cache;

// Semaphore to limit the number of concurrent readers (maximum 10 readers)
HANDLE readSemaphore;  // Semaphore to control the number of concurrent readers

// Helper to load the database from file (if necessary)
void loadDatabaseFromFile() {
    std::ifstream inFile(DATABASE_FILE);
    if (!inFile) {
        std::cerr << "Database file not found. Starting with an empty database.\n";
        return;
    }

    std::string key, value;
    while (std::getline(inFile, key) && std::getline(inFile, value)) {
        cache[key] = value;  // Populate in-memory cache
    }
}

// Helper to save the database to the file
void saveDatabaseToFile() {
    std::ofstream outFile(DATABASE_FILE, std::ios::trunc);
    if (!outFile) {
        std::cerr << "Failed to open database file for saving.\n";
        return;
    }

    for (const auto& entry : cache) {
        outFile << entry.first << "\n" << entry.second << "\n";  // Key-value pairs in separate lines
    }
}

// Process CRUD operations (CREATE, READ, UPDATE, DELETE)
void processOperation(const std::string& operation, char* response) {
    std::istringstream iss(operation);
    std::string cmd, key, value;

    iss >> cmd >> key;  // Read command and key
    if (cmd == "CREATE" || cmd == "UPDATE") {
        std::getline(iss, value);  // Get the value for CREATE or UPDATE
        {
            std::unique_lock<std::shared_mutex> lock(dbMutex);  // Exclusive lock for writing
            cache[key] = value;  // Modify the in-memory cache
            saveDatabaseToFile();  // Persist the changes to file
        }
        snprintf(response, 512, "SUCCESS: %s for %s", cmd.c_str(), key.c_str());
    } else if (cmd == "READ") {
        // Acquire the semaphore (limit the number of concurrent readers)
        WaitForSingleObject(readSemaphore, INFINITE);  // Block until a semaphore slot is available
        {
            std::shared_lock<std::shared_mutex> lock(dbMutex);  // Shared lock for reading
            if (cache.find(key) != cache.end()) {
                snprintf(response, 512, "READ: %s => %s", key.c_str(), cache[key].c_str());
            } else {
                snprintf(response, 512, "ERROR: Key %s not found", key.c_str());
            }
        }
        // Release the semaphore after reading
        ReleaseSemaphore(readSemaphore, 1, nullptr);  // Release one slot for another reader
    } else if (cmd == "DELETE") {
        {
            std::unique_lock<std::shared_mutex> lock(dbMutex);  // Exclusive lock for writing
            if (cache.erase(key)) {
                saveDatabaseToFile();  // Persist the changes to file
                snprintf(response, 512, "SUCCESS: DELETE %s", key.c_str());
            } else {
                snprintf(response, 512, "ERROR: Key %s not found", key.c_str());
            }
        }
    } else {
        snprintf(response, 512, "ERROR: Unknown command");
    }
}

int main() {
    // Create a memory-mapped file for inter-process communication
    HANDLE hFileMapping = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        MAPPED_FILE_SIZE,
        MEMORY_MAPPED_FILE_NAME);

    if (!hFileMapping) {
        std::cerr << "Failed to create memory-mapped file: " << GetLastError() << "\n";
        return 1;
    }

    SharedData* sharedData = (SharedData*)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, MAPPED_FILE_SIZE);
    if (!sharedData) {
        std::cerr << "Failed to map view of file: " << GetLastError() << "\n";
        CloseHandle(hFileMapping);
        return 1;
    }

    // Load the initial database from file to memory (cache)
    loadDatabaseFromFile();

    // Create a semaphore to limit the number of concurrent readers (maximum 10)
    readSemaphore = CreateSemaphore(
        nullptr,            // No security attributes
        10,                 // Initial count (allow 10 concurrent readers)
        10,                 // Maximum count (maximum 10 concurrent readers)
        nullptr);           // No name for the semaphore

    if (readSemaphore == nullptr) {
        std::cerr << "Failed to create semaphore: " << GetLastError() << "\n";
        return 1;
    }

    std::cout << "Database server running. Waiting for requests...\n";

    while (true) {
        if (sharedData->requestReady) {
            sharedData->requestReady = false;  // Mark request as processed

            // Process the operation and provide a response
            processOperation(sharedData->request, sharedData->response);

            sharedData->responseReady = true;  // Notify the client that the response is ready
        }
        Sleep(10);  // Avoid busy-waiting
    }

    // Clean up
    UnmapViewOfFile(sharedData);
    CloseHandle(hFileMapping);
    CloseHandle(readSemaphore);  // Close the semaphore handle

    return 0;
}
