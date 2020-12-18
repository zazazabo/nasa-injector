#pragma once
#include "../no_alloc/no_alloc.hpp"

int atexit(void (*func) (void));
int memcmp(const void* buf1, const void* buf2, size_t count);
void* memcpy(void* memTo, const void* memFrom, size_t size);
void* memmove(void* dst, const void* src, size_t count);
void* memset(void* dest, int c, size_t count);