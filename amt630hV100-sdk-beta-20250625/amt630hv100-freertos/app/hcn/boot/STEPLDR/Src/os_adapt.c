#include "os_adapt.h"

void *malloc(size_t size)
{
	return pvPortMalloc(size);
}

void free(void *ptr)
{
	vPortFree(ptr);
}
