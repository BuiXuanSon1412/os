#include <windows.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <sstream>
#include <random>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>
#include <cstring> // For std::strcpy

const wchar_t* MEMORY_MAPPED_FILE_NAME = L"MyMappedFile";
const size_t MAPPED_FILE_SIZE = 1024 * 1024; // 1 MB

struct SharedData {
    char request[512];
    char response[512];
    bool requestReady;
    bool responseReady;
};

std::mutex coutMutex; // Mutex for synchronizing std::cout

// Function to simulate a random CRUD operation
std::string generateRandomOperation() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 4); // Randomly choose between 1 to 4 (CRUD)
    std::uniform_int_distribution<> keyDist(1, 100); // Random keys from 1 to 100
    std::uniform_int_distribution<> valueDist(1, 100); // Random values from 1 to 100

    int operationType = dist(gen);
    int key = keyDist(gen);
    int value = valueDist(gen);

    std::stringstream ss;
    switch (operationType) {
    case 1: // CREATE
        ss << "CREATE " << key << " value_" << value;
        break;
    case 2: // READ
        ss << "READ " << key;
        break;
    case 3: // UPDATE
        ss << "UPDATE " << key << " value_" << value;
        break;
    case 4: // DELETE
        ss << "DELETE " << key;
        break;
    default:
        break;
    }
    return ss.str();
}

// Function to simulate the client behavior
void clientWorker(int clientID, HANDLE hFileMapping) {
    SharedData* sharedData = (SharedData*)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, MAPPED_FILE_SIZE);
    if (!sharedData) {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cerr << "Client " << clientID << " failed to map memory: " << GetLastError() << "\n";
        return;
    }

    std::string operation;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10; ++i) { // Simulate 10 operations per client
        // Wait until the server is ready to accept a request
        while (sharedData->requestReady) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Avoid busy-waiting
        }

        // Generate a random CRUD operation
        operation = generateRandomOperation();
        
        // Copy the generated operation string into the shared memory
        std::strcpy(sharedData->request, operation.c_str()); // Correct usage of std::strcpy
        
        sharedData->requestReady = true; // Indicate that a request is ready

        // Wait for the server's response
        while (!sharedData->responseReady) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Avoid busy-waiting
        }

        // Output the server's response
        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "Client " << clientID << " received: " << sharedData->response << "\n";
        }
        sharedData->responseReady = false; // Reset the response flag
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Client " << clientID << " completed all operations in " << elapsed.count() << " seconds.\n";
    }

    UnmapViewOfFile(sharedData);
}

int main() {
    // Create a memory-mapped file for inter-process communication
    HANDLE hFileMapping = OpenFileMappingW(
        FILE_MAP_ALL_ACCESS, // Open for reading and writing
        FALSE,               // Do not inherit the handle
        MEMORY_MAPPED_FILE_NAME);

    if (!hFileMapping) {
        std::cerr << "Failed to open memory-mapped file: " << GetLastError() << "\n";
        return 1;
    }

    // Simulate 100 concurrent clients (threads)
    const int numClients = 100;
    std::vector<std::thread> clientThreads;

    for (int i = 0; i < numClients; ++i) {
        clientThreads.emplace_back(clientWorker, i + 1, hFileMapping);
    }

    // Wait for all threads to finish
    for (auto& t : clientThreads) {
        t.join();
    }

    CloseHandle(hFileMapping);

    return 0;
}
