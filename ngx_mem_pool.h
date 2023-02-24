#pragma once
#include <iostream>
#include <string>
#include <memory>

/*
*	��ֲNginxԴ���룬ʹ����������˼����ʵ��
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
	u_char* last;					//��ǰ�ڴ�������λ�ã�����һ�οɷ����ڴ����ʼλ��
	u_char* end;					//�ڴ�ؽ���λ��
	NgxPool* next;					//���ӵ���һ���ڴ��
	ngx_uint_t failed;				//ͳ�Ƹ��ڴ�ز��������������Ĵ���
};

struct NgxLargeData
{
	NgxLargeData* next;		// ��һ������ڴ�
	void* alloc;		// ����Ĵ���ڴ�ռ�
};

struct NgxCleanUp {
	ngx_pool_cleanup_pt handler; // void(void*)
	void* data;
	NgxCleanUp* next;
};

struct NgxPool
{
	NgxData d;						//���ݿ�
	size_t max;						//���ݿ��С����С���ڴ�����ֵ
	NgxPool* current;				//���浱ǰ�ڴ�ֵ
	NgxLargeData* large;			//�������ڴ��ã�������max���ڴ�����
	NgxCleanUp* cleanup;			//����һЩ�ڴ���ͷŵ�ʱ��ͬʱ�ͷŵ���Դ
};

class NgxMemoryPool {
private:
	void* AllocBlock(size_t size);
	void* AllocLarge(size_t size);
public:
	NgxMemoryPool() = default;
	~NgxMemoryPool() = default;
	void Create(size_t size); // �����ڴ��
	void Destory();
	void Reset();
	void* Alloc(size_t size); // ��������ڴ棬�ڴ����
	void Dealloc(void* p);
	NgxCleanUp* AddCleanup(size_t size); // ����ⲿ��Դ
private:
	NgxPool* m_pool;
};


