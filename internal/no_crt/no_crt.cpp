#include "no_crt.h"

int atexit(void (*func) (void))
{
	func();
	return NULL;
}

int memcmp(const void* buf1, const void* buf2, size_t count)
{
	if (!count)
		return(0);

	while (--count && *(char*)buf1 == *(char*)buf2)
	{
		buf1 = (char*)buf1 + 1;

		buf2 = (char*)buf2 + 1;

	}
	return(*((unsigned char*)buf1) - *((unsigned char*)buf2));

}

void* memcpy(void* memTo, const void* memFrom, size_t size)
{
	if (!memTo || !memFrom)
		return nullptr;
	const char* temFrom = (const char*)memFrom;
	char* temTo = (char*)memTo;
	while (size-- > 0)
		*temTo++ = *temFrom++;
	return memTo;
}

void* memmove(void* dst, const void* src, size_t count)
{
	if (!dst || !src)
		return nullptr;
	void* ret = dst;
	if (dst <= src || (char*)dst >= ((char*)src + count))
	{
		while (count--)
		{
			*(char*)dst = *(char*)src;
			dst = (char*)dst + 1;
			src = (char*)src + 1;
		}
	}
	else
	{

		dst = (char*)dst + count - 1;
		src = (char*)src + count - 1;
		while (count--)
		{
			*(char*)dst = *(char*)src;
			dst = (char*)dst - 1;
			src = (char*)src - 1;
		}
	}
	return ret;
}

void* memset(void* dest, int c, size_t count)
{
	char* bytes = (char*)dest;
	while (count--)
	{
		*bytes++ = (char)c;
	}
	return dest;
}