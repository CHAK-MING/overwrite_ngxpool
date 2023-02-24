#pragma once
#include <iostream>
#include <string>
#include <memory>

/*
*	移植Nginx源代码，使用面向对象的思想来实现
* 
*/

using u_char = unsigned char;
using ngx_uint_t = uintptr_t;
typedef void (*ngx_pool_cleanup_pt)(void* data);

const ngx_uint_t ngx_pagsize = 4 * 1024; // 4kb
const ngx_uint_t NGX_MAX_ALLOC_FROM_POOL = ngx_pagsize - 1;
const ngx_uint_t NGX_DEFAULT_POOL_SIZE = 16 * 1024;
const ngx_uint_t NGX_POOL_ALIGNMENT = 16;
const ngx_uint_t NGX_ALIGNMENT = sizeof(ngx_uint_t);

#define ngx_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define ngx_align_ptr(p, a)                                                   \
    (u_char *) (((ngx_uint_t) (p) + ((ngx_uint_t) a - 1)) & ~((ngx_uint_t) a - 1))
#define ngx_malloc malloc
#define ngx_free free

struct NgxPool;

struct NgxData
{
	u_char* last;					//当前内存分配结束位置，即下一段可分配内存的起始位置
	u_char* end;					//内存池结束位置
	NgxPool* next;					//链接到下一个内存池
	ngx_uint_t failed;				//统计该内存池不能满足分配请求的次数
};

struct NgxLargeData
{
	NgxLargeData* next;		// 下一个大块内存
	void* alloc;		// 分配的大块内存空间
};

struct NgxCleanUp {
	ngx_pool_cleanup_pt handler; // void(void*)
	void* data;
	NgxCleanUp* next;
};

struct NgxPool
{
	NgxData d;						//数据块
	size_t max;						//数据块大小，即小块内存的最大值
	NgxPool* current;				//保存当前内存值
	NgxLargeData* large;			//分配大块内存用，即超过max的内存请求
	NgxCleanUp* cleanup;			//挂载一些内存池释放的时候，同时释放的资源
};

class NgxMemoryPool {
private:
	void* AllocBlock(size_t size);
	void* AllocLarge(size_t size);
public:
	NgxMemoryPool() = default;
	~NgxMemoryPool() = default;
	void Create(size_t size); // 创建内存池
	void Destory();
	void Reset();
	void* Alloc(size_t size); // 分配多少内存，内存对齐
	void Dealloc(void* p);
	NgxCleanUp* AddCleanup(size_t size); // 添加外部资源
private:
	NgxPool* m_pool;
};


