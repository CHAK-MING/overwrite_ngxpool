#pragma once
#include <cstddef>
#include <cstdint>

/*
 *	基于Nginx源码，使用C++实现内存池
 *
 */

using u_char = unsigned char;
using ngx_uint_t = uintptr_t;
typedef void (*ngx_pool_cleanup_pt)(void* data);

const ngx_uint_t ngx_pagsize = 4 * 1024;  // 4kb
const ngx_uint_t NGX_MAX_ALLOC_FROM_POOL = ngx_pagsize - 1;
const ngx_uint_t NGX_DEFAULT_POOL_SIZE = 16 * 1024;
const ngx_uint_t NGX_POOL_ALIGNMENT = 16;
const ngx_uint_t NGX_ALIGNMENT = sizeof(ngx_uint_t);

#define ngx_align(d, a) (((d) + (a - 1)) & ~(a - 1))
#define ngx_align_ptr(p, a) \
  (u_char*)(((ngx_uint_t)(p) + ((ngx_uint_t)a - 1)) & ~((ngx_uint_t)a - 1))
#define ngx_malloc malloc
#define ngx_free free

struct NgxPool;

struct NgxData {
  u_char* last;       // 当前内存分配的结束位置，即下一次可分配的起始位置
  u_char* end;        // 内存块的结束边界
  NgxPool* next;      // 指向链表中的下一个内存块
  ngx_uint_t failed;  // 当前内存块分配失败的次数统计
};

struct NgxLargeData {
  NgxLargeData* next;  // 指向下一个内存块
  void* alloc;         // 分配的内存块
};

struct NgxCleanUp {
  ngx_pool_cleanup_pt handler;  // void(void*)
  void* data;
  NgxCleanUp* next;
};

struct NgxPool {
  NgxData d;            // 数据区，内嵌了第一个内存块的元数据
  size_t max;           // 小内存的最大值
  NgxPool* current;     // 指向当前用于分配的内存块
  NgxLargeData* large;  // 管理大内存块的链表头指针
  NgxCleanUp* cleanup;  // 清理回调函数的链表头指针
};

class NgxMemoryPool {
 private:
  void* AllocBlock(size_t size);
  void* AllocLarge(size_t size);

 public:
  NgxMemoryPool() = default;
  ~NgxMemoryPool() = default;
  bool Create(size_t size);  // 创建内存池
  void Destory();
  void Reset();
  void* Alloc(size_t size);  // 分配内存
  void Dealloc(void* p);
  NgxCleanUp* AddCleanup(size_t size);  // 添加清理回调函数
 private:
  NgxPool* m_pool;
};
