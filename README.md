# Shared memory in Inter-process Communication

## Inter-process Communication
### 1. Definition

**Inter-Process Communication (IPC)** refers to mechanisms that allow processes to communicate and share data with each other. IPC is essential in operating systems to enable cooperation between multiple processes, whether they are running on the same system or distributed across different systems.

>**Key Features of IPC**:
1. **Data Exchange:** Facilitates the transfer of data between processes.
2. **Synchronization:** Ensures processes work in a coordinated way without interfering with each other.
3. **Resource Sharing:** Helps processes access shared resources efficiently.
4. **Process Management:** Enables processes to communicate status or control information.

### 2. Common IPC Approaches
| Approach    | Description |
| ----------- | ----------- |
|Pipes| Communication via a unidirectional or bidirectional data stream.|
|Message Queues| Exchange messages asynchronously using a queue.|
|Shared Memory| Directly share a memory segment between processes for high-speed communication.|
|Sockets|Enable communication between processes locally or over a network.|
|Signals|Asynchronous event notifications between processes.|
|Remote Procedure Calls (RPCs)|Abstract function calls to communicate between processes.|
|File-Based IPC|Use files as a medium to share data between processes.|


## Shared Memory in IPC
### 1. Definition
**Shared Memory** is an Inter-Process Communication (IPC) mechanism that allows multiple processes to access a common region of memory for exchanging data. It provides a fast and efficient way for processes to communicate since data does not need to be copied between processes, as is required in other IPC mechanisms
>**Key Characteristics of Shared Memory in IPC:**
1. **High-Speed Communication:** Shared memory allows processes to access and modify data directly in memory, which is much faster than other IPC methods that involve data copying.
2. **No Data Copying:** Unlike message passing or file-based IPC, shared memory does not involve copying data between user space and kernel space.
3. **Synchronization Needed:** Since multiple processes can access the same memory, synchronization mechanisms (like semaphores, mutexes, or condition variables) are often required to prevent race conditions and ensure data integrity.

### 2. Shared Memory in Windows
In Windows, shared memory is typically implemented using **memory-mapped files**. Shared memory in this context allows multiple processes to access the same region of memory, either by mapping a file or a memory block directly into the address space of the processes.

>**Memory-mapped Files service in Windows:**

<div style="text-align: center;">
    <img src="assets/arch-mmap.png" alt="Description" width="400">
</div>

-  In Windows, the **CreateFileMapping** function is used to create a memory-mapped file or shared memory region. The memory-mapped file can either back a real file on disk or be backed by virtual memory without any associated file (just using INVALID_HANDLE_VALUE).
- The **MapViewOfFile** function is used to map a portion of the file or memory object into the virtual address space of the calling process.
- While Windows provides **no automatic synchronization** for shared memory, **synchronization primitives** like mutexes, semaphores, or critical sections are often used to prevent race conditions.

>**Usage of Memory-mapped file service of Windows in IPC**

#### **Process A:** 

    1. OPTIONAL: OPEN a file for read/write access
    OPEN the file (e.g., "shared_memory.dat") with desired access permissions (read/write)
    IF file does not exist:
        CREATE the file (e.g., with a specific size, such as 1MB)

    2. CREATE a file mapping object
    CREATE a memory-mapped file mapping object for the file
    or CREATE a memory-mapped region with a unique name
    SPECIFY the size of the mapping (size of the file or a specific region)

    3. MAP the file into memory
    MAP the file into memory with desired access permissions (read/write)
    STORE the pointer to the mapped memory region

    4. WRITE data to the memory-mapped file
    WRITE the data to the mapped memory region (e.g., setting a value or struct)

    5. OPTIONAL: SIGNAL other processes (e.g., via events or semaphores) to notify that data is ready
    
    6. UNMAP the file from memory when done
    UNMAP the memory from the address space

    7. CLOSE the mapping and file handles to clean up resources


#### **Process B:**

    1. OPEN the same shared memory file for read/write access
    OPEN the same file (e.g., "shared_memory.dat") for read/write access
    
    2. CREATE a file mapping object for the same file
    CREATE a memory-mapped file mapping object with the same size as Process A

    3. MAP the file into memory
    MAP the same file into memory with desired access permissions (read/write)
    STORE the pointer to the mapped memory region

    4. READ data from the memory-mapped file
    READ the data from the mapped memory region (e.g., check if data is available)

    5. OPTIONAL: SIGNAL Process A that data has been read or processed (e.g., via events or semaphores)

    6. UNMAP the file from memory when done
    UNMAP the memory from the address space

    7. CLOSE the mapping and file handles to clean up resources

## Performance   
>Performance of Shared Memory method compared to the others

### Memory-mapped file
<p>
    <img src="assets/mmap-writer.PNG" alt="Image 1" width="250">
    <img src="assets/mmap-reader.PNG" alt="Image 2" width="250">
</p>

### Named pipe
<p>
    <img src="assets/pipe-writer.PNG" alt="Image 1" width="250">
    <img src="assets/pipe-reader.PNG" alt="Image 2" width="250">
</p>

### File-based IPC
<p>
    <img src="assets/file-writer.PNG" alt="Image 1" width="250">
    <img src="assets/file-reader.PNG" alt="Image 2" width="250">
</p>

### Summary
| **Aspect**              | **Static File**                          | **Memory-Mapped File**                  | **Named Pipe**                         |
|-------------------------|------------------------------------------|-----------------------------------------|----------------------------------------|
| **I/O Latency**          | High (due to disk I/O)                   | Low (close to memory access)            | Very Low (memory-based)                |
| **Data Transfer Speed**  | Moderate (disk access speed)             | Very Fast (direct memory access)        | Fast (kernel-buffered in memory)       |
| **Overhead**             | High (disk I/O, file locking)            | Low to Moderate (mapping, synchronization) | Low (kernel buffers)                   |
| **Concurrency Handling** | Requires external synchronization (locks) | OS manages mapping, but requires synchronization by processes | Built-in synchronization (but needs care in complex cases) |
| **Ease of Use**          | Simple, but slower                      | More complex, but very fast             | Easy to use for message-based communication |
| **Persistence**          | Yes (data is stored on disk)             | No (data is not persistent, unless backed by a file) | No (data exists only during communication) |
| **Suitability**          | Data persistence, low-volume communication | Large-scale, high-performance, fast data sharing | Real-time message-based communication |

## Application of Shared Memory in IPC
### Applications of Shared Memory in IPC

1. **High-Performance Data Sharing**
   - Used in real-time applications like multimedia processing and high-frequency trading.
   - Avoids data copying overhead for fast communication.

2. **Database Systems**
   - Implements buffer pools or caching mechanisms.
   - Allows concurrent access to in-memory data structures.

3. **Game Development**
   - Shares game state between processes like physics, AI, and rendering engines.
   - Reduces latency for real-time updates.

4. **Producer-Consumer Problem**
   - Acts as a circular buffer for intermediate data (e.g., logging, video encoding).
   - Facilitates asynchronous data sharing.

5. **Parallel Computing**
   - Shares large datasets between processes in scientific simulations or machine learning.
   - Enables efficient data partitioning without duplication.

6. **Real-Time Systems**
   - Synchronizes access to sensor data in robotics or avionics.
   - Ensures low-latency updates for time-critical tasks.

7. **Video Streaming and Processing**
   - Shares large video frames between decoding, encoding, and streaming processes.
   - Avoids bottlenecks from copying.

8. **Data Visualization**
   - Allows visualization tools to access processed data efficiently.
   - Avoids memory duplication.

---

### Advantages
- **Speed**: Faster than other IPC methods as no kernel intervention is needed.
- **Efficiency**: Conserves memory and reduces overhead.

### Challenges
- **Synchronization**: Requires semaphores or mutexes to prevent race conditions.
- **Complexity**: Harder to implement compared to simpler IPC mechanisms.

### Demostration

