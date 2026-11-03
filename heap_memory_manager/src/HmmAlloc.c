/*
    File : HmmAlloc.c
    Desc : Heap Memory Manager allocation functions
*/
#include "hmmneeds.h"
#include "Hmm.h"
#include <string.h>

blkheader_t *freelisthead = NULL;
blkheader_t *lastheapblk = NULL;
unsigned char *heapend = NULL;

void *HmmAlloc(size_t size)
{
  if (size == 0)
  {
    return NULL;
  }
  if (size > (SIZE_MAX - 15 - OVERHEADSIZE))
  {
    return NULL;
  }
  size_t needsize = roundto16(size);
  blkheader_t *blk = searchbestfit(needsize);
  if (blk == NULL)
  {
    blk = heapgrow(needsize);
    if (blk == NULL)
    {
      return NULL;
    }
  }
  removefreeblk(blk);
  splitblk(blk, needsize);
  return (void *)((unsigned char *)blk + OVERHEADSIZE);
}

void *hmmCalloc(size_t nmemb, size_t size)
{
  if (nmemb != 0 && size > SIZE_MAX / nmemb)
  {
    return NULL;
  }
  size_t totalsize = nmemb * size;
  void *ptr = HmmAlloc(totalsize);
  if (ptr == NULL)
  {
    return NULL;
  }
  memset(ptr, 0, totalsize);
  return ptr;
}

void *hmmReAlloc(void *ptr, size_t reqSize)
{
  if (ptr == NULL)
  {
    return HmmAlloc(reqSize);
  }
  if (reqSize == 0)
  {
    HmmFree(ptr);
    return NULL;
  }
  blkheader_t *blk = (blkheader_t *)((unsigned char *)ptr - OVERHEADSIZE);
  size_t oldsize = getrealsize(blk);
  if (oldsize >= reqSize)
  {
    return ptr;
  }
  void *newptr = HmmAlloc(reqSize);
  if (newptr == NULL)
  {
    return NULL;
  }
  memcpy(newptr, ptr, oldsize);
  HmmFree(ptr);
  return newptr;
}

size_t hmmSize(void *ptr)
{
  if (ptr == NULL)
  {
    return 0U;
  }
  blkheader_t *blk = (blkheader_t *)((unsigned char *)ptr - OVERHEADSIZE);
  return getrealsize(blk);
}

size_t roundto16(size_t size)
{
  return (size + 15) & ~((size_t)15);
}

/* Inserts blk at the front of the free list and marks it free. */
void insertfreeblk(blkheader_t *blk)
{
  if (freelisthead == NULL)
  {
    freelisthead = blk;
    blk->sharedmem.pointers.next = NULL;
    blk->sharedmem.pointers.prev = NULL;
  }
  else
  {
    blk->sharedmem.pointers.next = freelisthead;
    blk->sharedmem.pointers.prev = NULL;
    freelisthead->sharedmem.pointers.prev = blk;
    freelisthead = blk;
  }
  setfree(blk);
}

/* Unlinks blk from the free list and marks it allocated. blk must be in the list. */
void removefreeblk(blkheader_t *blk)
{
  if (isfree(blk) == 0)
  {
    return;
  }
  if (freelisthead == blk)
  {
    freelisthead = blk->sharedmem.pointers.next;
    if (blk->sharedmem.pointers.next != NULL)
    {
      blk->sharedmem.pointers.next->sharedmem.pointers.prev = NULL;
    }
    blk->sharedmem.pointers.next = NULL;
    blk->sharedmem.pointers.prev = NULL;
  }
  else if (blk->sharedmem.pointers.next == NULL)
  {
    blk->sharedmem.pointers.prev->sharedmem.pointers.next = NULL;
    blk->sharedmem.pointers.prev = NULL;
  }
  else
  {
    blk->sharedmem.pointers.prev->sharedmem.pointers.next = blk->sharedmem.pointers.next;
    blk->sharedmem.pointers.next->sharedmem.pointers.prev = blk->sharedmem.pointers.prev;
    blk->sharedmem.pointers.next = NULL;
    blk->sharedmem.pointers.prev = NULL;
  }
  setallocated(blk);
}

/* Returns the smallest free block that fits needsize (already rounded), or NULL. */
blkheader_t *searchbestfit(size_t needsize)
{
  blkheader_t *current = freelisthead;
  blkheader_t *bestfit = NULL;

  while (current != NULL)
  {
    size_t currentsize = getrealsize(current);
    if (currentsize >= needsize)
    {
      if (bestfit == NULL || currentsize < getrealsize(bestfit))
      {
        bestfit = current;
      }
    }
    current = current->sharedmem.pointers.next;
  }

  return bestfit;
}

/* Splits off the unused tail of blk as a free block. blk must be out of the list. */
void splitblk(blkheader_t *blk, size_t needsize)
{
  size_t currentsize = getrealsize(blk);
  if (currentsize >= needsize + OVERHEADSIZE + MINDATASIZE)
  {
    size_t newsize = currentsize - needsize - OVERHEADSIZE;
    blkheader_t *newblk = (blkheader_t *)((unsigned char *)blk + OVERHEADSIZE + needsize);
    newblk->size = 0;
    setsize(newblk, newsize);
    newblk->prevsize = needsize;
    unsigned char *nextblkptr = (unsigned char *)newblk + OVERHEADSIZE + newsize;
    if (nextblkptr < heapend)
    {
      blkheader_t *nextblk = (blkheader_t *)nextblkptr;
      nextblk->prevsize = newsize;
    }
    else
    {
      lastheapblk = newblk;
    }

    setsize(blk, needsize);
    insertfreeblk(newblk);
  }
}

/* Gets more memory from sbrk; returns a free block of at least needsize, or NULL. */
blkheader_t *heapgrow(size_t needsize)
{
  size_t growsize = (needsize + OVERHEADSIZE > HEAPGROWCHUNK) ? (needsize + OVERHEADSIZE) : HEAPGROWCHUNK;
  size_t padding = 0;
  if (heapend == NULL)
  {
    uintptr_t firstbrk = (uintptr_t)sbrk(0);
    padding = (roundto16(firstbrk) - firstbrk);
  }
  if (growsize + padding > (size_t)INTPTR_MAX)
  {
    return NULL;
  }
  unsigned char *newbrk = (unsigned char *)sbrk(growsize + padding);
  if (newbrk == (unsigned char *)-1)
  {
    return NULL;
  }
  blkheader_t *oldlastheapblk = lastheapblk;
  blkheader_t *newblk = (blkheader_t *)(newbrk + padding);
  newblk->size = 0;
  setsize(newblk, growsize - OVERHEADSIZE);
  newblk->prevsize = (oldlastheapblk != NULL) ? getrealsize(oldlastheapblk) : 0;
  heapend = newbrk + growsize + padding;
  lastheapblk = newblk;
  insertfreeblk(newblk);
  blkheader_t *finalblk = newblk;
  if (oldlastheapblk != NULL && isfree(oldlastheapblk))
  {
    if ((unsigned char *)oldlastheapblk + OVERHEADSIZE + getrealsize(oldlastheapblk) == (unsigned char *)newblk)
    {
      mergeblk(oldlastheapblk);
      finalblk = oldlastheapblk;
    }
  }

  return finalblk;
}

/* Gives the free tail of the heap back to the OS with sbrk, keeping one chunk. */
void heapshrink(void)
{
  if (lastheapblk == NULL || !isfree(lastheapblk))
  {
    return;
  }
  size_t tailsize = getrealsize(lastheapblk);
  if (tailsize < HEAPSHRINKTHRESHOLD)
  {
    return;
  }
  if ((unsigned char *)sbrk(0) != heapend)
  {
    return;
  }
  size_t releasesize = tailsize - HEAPGROWCHUNK;
  unsigned char *oldbrk = (unsigned char *)sbrk(-(intptr_t)releasesize);
  if (oldbrk == (unsigned char *)-1)
  {
    return;
  }
  setsize(lastheapblk, HEAPGROWCHUNK);
  heapend = heapend - releasesize;
}
/* Merges blk (free, in the list) with the next block in memory if that one is free. */
void mergeblk(blkheader_t *blk)
{
  if (blk == NULL || isfree(blk) == 0)
  {
    return;
  }
  unsigned char *nextblkptr = (unsigned char *)blk + OVERHEADSIZE + getrealsize(blk);
  if (nextblkptr >= heapend)
  {
    return;
  }
  blkheader_t *nextblk = (blkheader_t *)nextblkptr;
  if (!isfree(nextblk))
  {
    return;
  }
  removefreeblk(nextblk);
  size_t newsize = getrealsize(blk) + OVERHEADSIZE + getrealsize(nextblk);
  setsize(blk, newsize);
  unsigned char *afternewptr = (unsigned char *)blk + OVERHEADSIZE + newsize;
  if (afternewptr < heapend)
  {
    blkheader_t *afternewblk = (blkheader_t *)afternewptr;
    afternewblk->prevsize = newsize;
  }
  else
  {
    lastheapblk = blk;
  }
}