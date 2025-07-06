#include "ngx_mem_pool.h"

#if defined(_WIN32)
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <iostream>

static void* memalign(size_t size, size_t alignment) {
  void* p;
#if defined(_WIN32)
  p = _aligned_malloc(size, alignment);
#else
  if (posix_memalign(&p, alignment, size) != 0) {
    p = nullptr;
  }
#endif
  return p;
}

void* NgxMemoryPool::AllocBlock(size_t size) {
  u_char* m;
  size_t psize;
  NgxPool *p, *newp, *current;

  psize = (size_t)(m_pool->d.end - (u_char*)m_pool->current);

  m = static_cast<u_char*>(memalign(psize, NGX_POOL_ALIGNMENT));
  if (m == nullptr) {
    return nullptr;
  }

  newp = (NgxPool*)m;

  newp->d.end = m + psize;
  newp->d.next = nullptr;
  newp->d.failed = 0;

  m += sizeof(NgxData);  // 表示第二个内存块的内存只需要存储NgxData结构体
  m = ngx_align_ptr(m, NGX_ALIGNMENT);
  newp->d.last = m + size;

  current = m_pool->current;

  for (p = current; p->d.next; p = p->d.next) {
    if (p->d.failed++ >
        4) {  // 如果当前内存块分配失败次数超过4次，则将current指向下一个内存块
      current = p->d.next;
    }
  }

  p->d.next = newp;

  m_pool->current = current ? current : newp;

  return m;
}

void* NgxMemoryPool::AllocLarge(size_t size) {
  void* p;
  ngx_uint_t n;
  NgxLargeData* large;

  p = ngx_malloc(size);
  if (p == nullptr) return nullptr;

  n = 0;

  for (large = m_pool->large; large; large = large->next) {
    if (large->alloc == nullptr)  // 遍历链表，找到第一个未分配的内存块
    {
      large->alloc = p;
      return p;
    }
    if (n++ > 3)  // 如果遍历3次都没有找到未分配的内存块，则直接退出
      break;
  }

  large = static_cast<NgxLargeData*>(Alloc(sizeof(NgxLargeData)));
  if (large == nullptr) {
    ngx_free(p);
    return nullptr;
  }

  large->alloc = p;
  large->next = m_pool->large;  // 将large指针指向链表头
  m_pool->large = large;

  return p;
}

bool NgxMemoryPool::Create(size_t size) {
  NgxPool* p;
  p = static_cast<NgxPool*>(memalign(size, NGX_POOL_ALIGNMENT));

  // 如果分配失败
  if (p == nullptr) {
    std::cerr << "memalign error" << std::endl;
    return false;
  }

  p->d.last = (u_char*)p + sizeof(NgxPool);
  p->d.end = (u_char*)p + size;
  p->d.next = nullptr;
  p->d.failed = 0;

  size = size - sizeof(NgxPool);
  p->max = std::max(size, NGX_MAX_ALLOC_FROM_POOL);

  p->current = p;  // 指向当前内存块
  p->large = nullptr;
  p->cleanup = nullptr;

  m_pool = p;
  return true;
}

void NgxMemoryPool::Destory() {
  if (m_pool == nullptr) {
    return;
  }

  NgxCleanUp* c;
  for (c = m_pool->cleanup; c; c = c->next) {
    if (c->handler) {
      c->handler(c->data);
    }
  }

  NgxLargeData* l;
  for (l = m_pool->large; l; l = l->next) {
    if (l->alloc) {
      ngx_free(l->alloc);
    }
  }

  NgxPool* current = m_pool;
  while (current) {
    NgxPool* next = current->d.next;
#if defined(_WIN32)
    _aligned_free(current);
#else
    free(current);
#endif
    current = next;
  }
  m_pool = nullptr;
}

void NgxMemoryPool::Reset() {
  NgxPool* p;
  NgxLargeData* l;

  // 释放大内存块
  for (l = m_pool->large; l; l = l->next) {
    if (l->alloc) ngx_free(l->alloc);
  }

  m_pool->large = nullptr;

  for (p = m_pool; p; p = p->d.next) {
    if (p == m_pool)
      p->d.last = reinterpret_cast<u_char*>(p) + sizeof(NgxPool);
    else
      p->d.last = reinterpret_cast<u_char*>(p) + sizeof(NgxData);
    p->d.failed = 0;
  }

  m_pool->current = m_pool;
  m_pool->large = nullptr;
}

void* NgxMemoryPool::Alloc(size_t size) {
  u_char* m;
  NgxPool* p;

  if (size <= m_pool->max) {
    p = m_pool->current;

    do {
      m = ngx_align_ptr(p->d.last, NGX_ALIGNMENT);

      if ((size_t)(p->d.end - m) >= size) {
        p->d.last = m + size;

        return m;
      }

      p = p->d.next;

    } while (p);

    return AllocBlock(size);
  }
  return AllocLarge(size);
}

// 释放内存
void NgxMemoryPool::Dealloc(void* p) {
  NgxLargeData* l;

  for (l = m_pool->large; l; l = l->next) {
    // 只释放alloc指针指向的内存块
    if (p == l->alloc) {
      ngx_free(l->alloc);
      l->alloc = nullptr;
    }
  }
}

NgxCleanUp* NgxMemoryPool::AddCleanup(size_t size) {
  NgxCleanUp* c;
  c = static_cast<NgxCleanUp*>(Alloc(sizeof(NgxCleanUp)));

  if (c == nullptr) return nullptr;

  if (size) {
    c->data = Alloc(size);
    if (c->data == nullptr)
      return nullptr;
    else
      c->data = nullptr;
  }

  c->handler = nullptr;
  c->next = m_pool->cleanup;
  m_pool->cleanup = c;

  return c;
}
