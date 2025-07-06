#include <chrono>
#include <iostream>
#include <vector>

#include "ngx_mem_pool.h"

// Constants for the benchmark
const int ALLOCATION_COUNT = 100000;
const size_t SMALL_ALLOC_SIZE = 128;  // Clearly a small allocation
const size_t LARGE_ALLOC_SIZE =
    8192;  // Clearly a large allocation ( > 4k page)

// Measures execution time of a given function
template <typename Func>
long long measure_time_ms(Func f) {
  auto start = std::chrono::high_resolution_clock::now();
  f();
  auto end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
      .count();
}

void benchmark_standard_malloc() {
  std::vector<void*> allocs;
  allocs.reserve(ALLOCATION_COUNT);

  for (int i = 0; i < ALLOCATION_COUNT; ++i) {
    if (i % 2 == 0) {
      allocs.push_back(malloc(SMALL_ALLOC_SIZE));
    } else {
      allocs.push_back(malloc(LARGE_ALLOC_SIZE));
    }
  }

  for (void* p : allocs) {
    free(p);
  }
}

void benchmark_ngx_pool() {
  NgxMemoryPool pool;
  if (!pool.Create(16 * 1024 * 1024)) {  // Create a large pool (16MB) to avoid
                                         // too many block allocations
    std::cerr << "Failed to create ngx_pool" << std::endl;
    return;
  }

  std::vector<void*> allocs;
  allocs.reserve(ALLOCATION_COUNT);

  for (int i = 0; i < ALLOCATION_COUNT; ++i) {
    if (i % 2 == 0) {
      allocs.push_back(pool.Alloc(SMALL_ALLOC_SIZE));
    } else {
      allocs.push_back(pool.Alloc(LARGE_ALLOC_SIZE));
    }
  }

  // With a memory pool, we don't free individual allocations.
  // We just destroy the pool. Destruction time includes freeing large
  // allocations.
  pool.Destory();
}

int main() {
  std::cout << "Running benchmarks with " << ALLOCATION_COUNT
            << " allocations...\n";
  std::cout << "--------------------------------------------------\n";

  long long malloc_time = measure_time_ms(benchmark_standard_malloc);
  std::cout << "Standard malloc/free time: " << malloc_time << " ms\n";

  long long ngx_pool_time = measure_time_ms(benchmark_ngx_pool);
  std::cout << "NgxMemoryPool alloc/destroy time: " << ngx_pool_time << " ms\n";

  std::cout << "--------------------------------------------------\n";

  if (ngx_pool_time < malloc_time) {
    std::cout << "Benchmark result: NgxMemoryPool was faster.\n";
  } else {
    std::cout << "Benchmark result: Standard malloc/free was faster.\n";
  }

  return 0;
}