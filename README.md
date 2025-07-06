# Nginx Memory Pool - C++14

A C++14 implementation of the Nginx memory pool, designed for high-performance applications requiring frequent memory allocations.

## Project Structure
```
overwrite_ngxpool/
├── src/                # Library source code (.cc)
├── include/            # Library headers (.h)
├── examples/           # Example usage
├── benchmark/          # Performance benchmark
├── CMakeLists.txt      # CMake build script
├── LICENSE             # MIT License
└── .gitignore          # Git ignore file
```

## How to Build

This project uses CMake.

```bash
# 1. Create a build directory
mkdir build
cd build

# 2. Configure the project
cmake ..

# 3. Build the library and examples
make
```

This will produce the library, the example executable `memory_pool_demo`, and the benchmark executable `memory_pool_benchmark` inside the `build/` directory.

## Basic Usage

```cpp
#include "ngx_mem_pool.h"
#include <iostream>

int main() {
    NgxMemoryPool pool;
    
    // 1. Create a memory pool
    if (!pool.Create(1024)) {
        std::cerr << "Failed to create pool" << std::endl;
        return 1;
    }
    
    // 2. Allocate memory
    // Small allocation from the pool
    void* p1 = pool.Alloc(128);
    
    // Large allocation (will use malloc directly)
    void* p2 = pool.Alloc(2048);
    
    std::cout << "Allocations successful" << std::endl;

    // 3. Destroy the pool
    // This releases all allocated memory, including small and large blocks,
    // and runs any registered cleanup handlers.
    pool.Destory();
    
    return 0;
}
```

## Advanced Usage - Cleanup Handlers

The memory pool can manage external resources (like file handles or `malloc`'d memory) via cleanup handlers.

```cpp
#include "ngx_mem_pool.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>


struct Data {
    char* ptr;
    FILE* file;
};

// Cleanup function for malloc'd memory
void cleanup_malloc(void* p) {
    if (p) {
        std::cout << "Freeing malloc'd pointer." << std::endl;
        free(p);
    }
}

// Cleanup function for a file handle
void cleanup_file(void* fp) {
    if (fp) {
        std::cout << "Closing file." << std::endl;
        fclose((FILE*)fp);
    }
}

int main() {
    NgxMemoryPool pool;
    pool.Create(512);
    
    // Allocate a struct from the pool
    Data* data = static_cast<Data*>(pool.Alloc(sizeof(Data)));
    
    // Allocate external resources
    data->ptr = (char*)malloc(12);
    strcpy(data->ptr, "hello world");
    data->file = fopen("test.txt", "w");
    
    // Register a cleanup handler for the malloc'd pointer
    NgxCleanUp* c1 = pool.AddCleanup(0); // 0 size, we provide the pointer
    c1->handler = cleanup_malloc;
    c1->data = data->ptr;
    
    // Register a cleanup handler for the file
    NgxCleanUp* c2 = pool.AddCleanup(0);
    c2->handler = cleanup_file;
    c2->data = data->file;
    
    // When Destory() is called, cleanup_malloc and cleanup_file will be invoked automatically.
    pool.Destory();
    
    return 0;
}
```

## How to Run Benchmark

After building the project, you can run the performance test:

```bash
cd build/
./memory_pool_benchmark
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
