/* 
    File : heapWrappers.c
    Desc : Wrappers for all the needed functions in the heap functions family
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Hmm.h"

void * malloc(size_t size)
{
    return HmmAlloc(size);
}

void free(void *ptr)
{
    HmmFree(ptr);
}

void *calloc(size_t nmemb, size_t size)
{
	return hmmCalloc(nmemb, size);
}

void* realloc(void *ptr, size_t size)
{
	return hmmReAlloc(ptr, size);
}