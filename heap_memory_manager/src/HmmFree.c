/*
    File : HmmFree.c
    Desc : Heap Memory Manager free function
*/

#include "Hmm.h"
#include "hmmneeds.h"

void HmmFree(void *ptr)
{
  if (ptr == NULL)
  {
    return;
  }
  blkheader_t *blk = (blkheader_t *)((unsigned char *)ptr - OVERHEADSIZE);
  if (isfree(blk))
  {
    return;
  }
  insertfreeblk(blk);
  mergeblk(blk);
  if (blk->prevsize != 0)
  {
    unsigned char *prevblkptr = (unsigned char *)blk - OVERHEADSIZE - blk->prevsize;
    blkheader_t *prevblk = (blkheader_t *)prevblkptr;
    if (isfree(prevblk))
    {
      mergeblk(prevblk);
    }
  }
  heapshrink();
}
