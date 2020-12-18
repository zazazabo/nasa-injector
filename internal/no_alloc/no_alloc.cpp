#include "no_alloc.hpp"

void* malloc(std::size_t size)
{
	return heap.allocate(size);
}

void free(void* ptr, std::size_t size)
{
	heap.deallocate((std::uint8_t*)ptr, size);
}

void* operator new(std::size_t size)
{
	return malloc(size);
}

void operator delete(void* ptr, unsigned __int64 size)
{
	free(ptr, size);
}