# Heap Memory Manager (HMM)

A custom heap allocator in C for Linux that replaces `malloc`, `free`, `calloc` and `realloc`. It gets memory from the OS with `sbrk`, tracks free blocks with an intrusive doubly linked free list, and uses best-fit allocation with block splitting and coalescing.

## Features

- Best-fit search over the free list
- Block splitting, with a minimum useful leftover
- merging with both the next and the previous block on `free`
- 16-byte header, 16-byte aligned user pointers
- Free-list pointers share memory with user data (union), so allocated blocks pay no pointer overhead
- Heap grown in 128 KB chunks to reduce `sbrk` calls
- Overflow checks in `malloc`, `calloc` and the `sbrk` request
- Works as a drop-in replacement through static linking or `LD_PRELOAD`

## Project layout

```
inc/Hmm.h          public interface (HmmAlloc, HmmFree, hmmCalloc, hmmReAlloc, hmmSize)
inc/hmmneeds.h     internal block header, helpers and declarations
src/HmmAlloc.c    allocation, free list, split, merge, heap growth, calloc, realloc, size
src/HmmFree.c     HmmFree
src/heapWrappers.c  malloc/free/calloc/realloc wrappers (provided, unchanged)
test/main.c        provided test program
test_stress.c      extra randomized stress test
Makefile
```

## Design

### Block layout

Every block starts with a 16-byte header, followed by the data area:

```
offset 0   size       data size in bytes; bit 0 is the free flag
offset 8   prevsize   data size of the previous block in memory (0 for the first block)
offset 16  data       user data, or the free-list pointers while the block is free
```

The user pointer is `block + 16`. Sizes are always rounded up to a multiple of 16, so the low 4 bits of `size` are always zero and bit 0 can carry the free flag. The minimum data size is 16 bytes so a free block always has room for the two list pointers.

### Union overlay

A free block stores `next` and `prev` in its data area. An allocated block is no longer in the list, so those same 16 bytes belong to the user. The allocator only reads the list pointers of a block whose free flag is set. `size` and `prevsize` live outside the union and are always valid.

### Allocation (`HmmAlloc`)

1. Return `NULL` for size 0 or for sizes that would overflow when rounded.
2. Round the size up to a multiple of 16 (once).
3. Search the free list for the smallest block that fits (best fit).
4. If none fits, grow the heap with `sbrk` (`heapgrow`).
5. Remove the block from the free list and split off the unused tail if the leftover can hold a header plus 16 data bytes.
6. Return `block + 16`.

### Free (`HmmFree`)

1. Ignore `NULL` and blocks already marked free.
2. Insert the block into the free list and mark it free.
3. Merge with the next block in memory if it is free.
4. If the previous block (found with `prevsize`) is free, merge the previous block with this one. This reuses the same "merge with next" routine, started from the previous block.

### Heap growth

The first call pads the start of the heap so it is 16-byte aligned. Each growth asks for at least 128 KB. If the last block of the heap is free and adjacent to the new memory, the two are merged.

### Calloc and realloc

- `hmmCalloc` checks `nmemb * size` for overflow, allocates, and zeroes the block.
- `hmmReAlloc` handles `NULL` and size 0, returns the same pointer if the block is already large enough, otherwise allocates, copies and frees the old block. If the new allocation fails, the old block is left untouched.

## Build

Requires `gcc` and `make` (`sudo apt install build-essential`).

```sh
make              # static library, shared library and test executable
make static_lib   # build/lib/libhmm.a
make shared_lib   # build/lib/libhmm.so
make test         # build/hmm_test
make clean
```

## Run the tests

```sh
make test
./build/hmm_test
```


## Use the allocator

Static linking:

```sh
make static_lib
gcc -Iinc example.c build/lib/libhmm.a -o example
```

With `LD_PRELOAD` (dynamically linked programs only):

```sh
make shared_lib
export HMM_LIB="$(pwd)/build/lib/libhmm.so"
LD_PRELOAD="$HMM_LIB" ls -la
LD_PRELOAD="$HMM_LIB" bash --noprofile --norc
```

Do not add the library to `/etc/ld.so.preload` or a login profile. If a preloaded program fails, run `unset LD_PRELOAD`.

## Limitations

- Not thread safe (no locking).
- Memory is never returned to the OS; freed memory stays in the free list.
- Assumes nothing else moves the program break between the allocator's `sbrk` calls.
- The double-free guard is best effort: it only works while the freed block's header is untouched.
- Best fit scans the whole free list, so allocation is O(n) in the number of free blocks.
- Do not build with `-fsanitize`, since it conflicts with a replaced `malloc`.
