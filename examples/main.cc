#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "ngx_mem_pool.h"

struct Data {
  char* ptr;
  FILE* pfile;
};

void func1(void* p) { free(p); }

void func2(void* fp) { fclose((FILE*)fp); }

int main() {
  NgxMemoryPool pool;
  if (!pool.Create(512)) {
    std::cerr << "Failed to create memory pool." << std::endl;
    return 1;
  }

  void* p1 = pool.Alloc(128);
  if (p1 == nullptr) {
    std::cerr << "ngx_pool alloc 128 bytes failed..." << std::endl;
    exit(2);
  }

  Data* p2 = static_cast<Data*>(pool.Alloc(512));
  if (p2 == nullptr) {
    std::cerr << "ngx_pool alloc 512 bytes failed..." << std::endl;
    exit(2);
  }

  p2->ptr = (char*)malloc(12);
  strcpy(p2->ptr, "hello world");
  p2->pfile = fopen("data.txt", "w");

  NgxCleanUp* c1 = pool.AddCleanup(sizeof(char*));
  c1->handler = func1;
  c1->data = p2->ptr;

  NgxCleanUp* c2 = pool.AddCleanup(sizeof(FILE*));
  c2->handler = func2;
  c2->data = p2->pfile;

  pool.Destory();
}