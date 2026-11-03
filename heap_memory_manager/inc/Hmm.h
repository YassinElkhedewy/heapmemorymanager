/*
    File : Hmm.h
    Desc : Heap Memory Manager include file
*/

#ifndef HMM_H
#define HMM_H

#include <stddef.h>

void HmmFree(void *ptr);

void *HmmAlloc(size_t size);

void *hmmCalloc(size_t nmemb, size_t size);

void *hmmReAlloc(void *ptr, size_t reqSize);

size_t hmmSize(void *ptr);

#endif /* HMM_H */