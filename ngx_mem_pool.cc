#include "ngx_mem_pool.h"

static void* memalign(size_t size, size_t alignment)
{
	void* p = _aligned_malloc(size, alignment);

	if (p == nullptr)
		return nullptr;

	return p;
}

void* NgxMemoryPool::AllocBlock(size_t size)
{
	u_char* m;
	size_t psize;
	NgxPool* p, *newp, *current;

	psize = (size_t)(m_pool->d.end - (u_char*)m_pool->current);

	m = static_cast<u_char*>(memalign(psize, NGX_POOL_ALIGNMENT));
	if (m == nullptr) {
		return nullptr;
	}

	newp = (NgxPool*)m;

	newp->d.end = m + psize;
	newp->d.next = nullptr;
	newp->d.failed = 0;

	m += sizeof(NgxData); // ��ʾ�ڶ����ڴ�صĿ�ֻ��Ҫ�洢NgxData������
	m = ngx_align_ptr(m, NGX_ALIGNMENT);
	newp->d.last = m + size;

	current = m_pool->current;

	for (p = current; p->d.next; p = p->d.next) {
		if (p->d.failed++ > 4) { // ���������ڴ����룬��ǰ���ڴ�ؿ鶼û�취����Ҫ�󣬳����Ĵ�֮�󣬾ͽ�currentָ����һ���ڴ�ؿ�
			current = p->d.next;
		}
	}

	p->d.next = newp;

	m_pool->current = current ? current : newp;

	return m;
}

void* NgxMemoryPool::AllocLarge(size_t size)
{
	void* p;
	ngx_uint_t n;
	NgxLargeData* large;

	p = ngx_malloc(size);
	if (p == nullptr)
		return nullptr;

	n = 0;

	for (large = m_pool->large; large; large = large->next)
	{
		if (large->alloc == nullptr) // ѭ���������ҵ��Ѿ����ͷŵĴ��ڴ��
		{
			large->alloc = p;
			return p;
		}
		if (n++ > 3) // ���ζ��Ҳ�����ֱ���˳�
			break;
	}

	large = static_cast<NgxLargeData*>(Alloc(sizeof(NgxLargeData)));
	if (large == nullptr)
	{
		ngx_free(p);
		return nullptr;
	}

	large->alloc = p;
	large->next = m_pool->large; // ͷ��
	m_pool->large = large;

	return p;
}

void NgxMemoryPool::Create(size_t size)
{
	NgxPool* p;
	p = static_cast<NgxPool*>(memalign(size, NGX_POOL_ALIGNMENT));

	// ����ʧ��
	if (p == nullptr)
	{
		std::cerr << "memalign error" << std::endl;
		return;
	}

	p->d.last = (u_char*)p + sizeof(NgxPool);
	p->d.end = (u_char*)p + size;
	p->d.next = nullptr;
	p->d.failed = 0;

	size = size - sizeof(NgxPool);
	p->max = std::max(size, NGX_MAX_ALLOC_FROM_POOL);

	p->current = p; // ָ���ڴ����ʼλ��
	p->large = nullptr;
	p->cleanup = nullptr;

	m_pool = p;
}

void NgxMemoryPool::Destory()
{
	NgxPool* p, * n;
	NgxLargeData* l;
	NgxCleanUp* c;

	// �����ⲿָ������
	for (c = m_pool->cleanup; c; c = c->next)
	{
		if (c->handler)
		{
			c->handler(c->data);
		}
	}

	// �Դ�����ݵ�����
	for (l = m_pool->large; l; l = l->next)
	{
		if (l->alloc)
		{
			ngx_free(l->alloc);
		}
	}

	for (p = m_pool->current, n = m_pool->d.next; ;p = n, n = n->d.next)
	{
		_aligned_free(p);

		if (n == nullptr)
			break;
	}
}

void NgxMemoryPool::Reset()
{
	NgxPool* p;
	NgxLargeData* l;

	// ���ͷŴ���ڴ�
	for (l = m_pool->large; l; l->next)
	{
		if (l->alloc)
			ngx_free(l->alloc);
	}

	m_pool->large = nullptr;

	for (p = m_pool->current; p; p = p->d.next)
	{
		if (p == m_pool->current)
			p->d.last = reinterpret_cast<u_char*>(p) + sizeof(NgxPool);
		else
			p->d.last = reinterpret_cast<u_char*>(p) + sizeof(NgxData);
		p->d.failed = 0;
	}

	m_pool->large = nullptr;
}

void* NgxMemoryPool::Alloc(size_t size)
{
	u_char* m;
	NgxPool* p;

	if (size <= m_pool->max) {

		p = m_pool->current;

		do {
			m = ngx_align_ptr(m_pool->d.last, NGX_ALIGNMENT); // �����ڴ�ָ�룬�ӿ��ȡ�ٶ� ������ƽ̨��أ�unsigned long 8

			if ((size_t)(m_pool->d.end - m) >= size) { // �ڴ�ؿ��е��ڴ��ַ����Ҫ������ڴ棬��ʾ�ܹ�����
				m_pool->d.last = m + size;

				return m;
			}

			p = m_pool->d.next;

		} while (p);

		return AllocBlock(size);
	}
	//size������Ҫ�ڴ����ݿ��ڴ������������ڴ�ռ�
	return AllocLarge(size);
}

// ���ƴ���ڴ���ͷ�
void NgxMemoryPool::Dealloc(void* p)
{
	NgxLargeData* l;

	for (l = m_pool->large; l; l = l->next)
	{
		// ֻ���ͷ�alloc������ԭ����ͷ���ṹ
		if (p == l->alloc)
		{
			ngx_free(l->alloc); 
			l->alloc = nullptr; 
		}
	}
}

NgxCleanUp* NgxMemoryPool::AddCleanup(size_t size)
{
	NgxCleanUp* c;
	c = static_cast<NgxCleanUp*>(Alloc(sizeof(NgxCleanUp)));

	if (c == nullptr)
		return nullptr;

	if (size)
	{
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
