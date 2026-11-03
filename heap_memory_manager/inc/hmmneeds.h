
#ifndef HMMNEEDS_H
#define HMMNEEDS_H
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

size_t roundto16(size_t size);

typedef struct blkheader blkheader_t;
typedef struct listpointers
{
    struct blkheader *next;
    struct blkheader *prev;
} listpointers_t;
struct blkheader
{
    size_t size;     /* data size, bit 0 = free flag */
    size_t prevsize; /* data size of previous block in memory, 0 if first */
    union sharedmem
    {
        listpointers_t pointers;
        unsigned char data[16];
    } sharedmem;
};

#define OVERHEADSIZE (offsetof(blkheader_t, sharedmem.data))
#define MINDATASIZE 16
#define HEAPGROWCHUNK (128 * 1024)
#define HEAPSHRINKTHRESHOLD (2 * HEAPGROWCHUNK)

static inline size_t getrealsize(blkheader_t *blk)
{
    return blk->size & ~(size_t)15;
}

static inline int isfree(blkheader_t *blk)
{
    return (int)(blk->size & 1);
}

static inline void setfree(blkheader_t *blk)
{
    blk->size |= 1;
}

static inline void setallocated(blkheader_t *blk)
{
    blk->size &= ~(size_t)1;
}
static inline void setsize(blkheader_t *blk, size_t size)
{
    blk->size = roundto16(size) | (blk->size & 1);
}

extern blkheader_t *freelisthead;
extern unsigned char *heapend;
extern blkheader_t *lastheapblk;
void insertfreeblk(blkheader_t *blk);
void removefreeblk(blkheader_t *blk);
blkheader_t *searchbestfit(size_t needsize);
blkheader_t *heapgrow(size_t needsize);
void splitblk(blkheader_t *blk, size_t needsize);
void mergeblk(blkheader_t *blk);
void heapshrink(void);
#endif // HMMNEEDS_H
