# Heap Memory Manager (HMM)

A custom heap allocator in C for Linux that replaces `malloc`, `free`, `calloc` and `realloc`. It gets memory from the OS with `sbrk`, tracks free blocks with an intrusive doubly linked free list, and uses best-fit allocation with block splitting, merging and heap shrinking.

Flowcharts for `HmmAlloc` and `HmmFree` are in [`HMM_Flowcharts.pdf`](HMM_Flowcharts.pdf).

## Features

- Best-fit search over the free list
- Block splitting, with a minimum useful leftover
- merging with both the next and the previous block on `free`
- 16-byte header and 16-byte aligned user pointers
- Free-list pointers share memory with user data (union), so allocated blocks pay no pointer overhead
- Heap grown in 128 KB chunks and shrunk only when a large free tail exists, to minimize `sbrk` calls
- Return values of `sbrk` are checked, and requests that would overflow are rejected
- Drop-in replacement through static linking or `LD_PRELOAD`

## Design

### Block layout

Every block starts with a 16-byte header, followed by the data area:

```
offset 0   size       data size in bytes; bit 0 is the free flag
offset 8   prevsize   data size of the previous block in memory (0 for the first block)
offset 16  data       user data, or the free-list pointers while the block is free
```

The user pointer is `block + 16`. Sizes are always rounded up to a multiple of 16, so the low 4 bits of `size` are always zero and bit 0 carries the free flag. The minimum data size is 16 bytes so a free block always has room for the two list pointers. The heap start is padded once so that every block, and therefore every user pointer, is 16-byte aligned.

### Union overlay

A free block stores `next` and `prev` in its data area. An allocated block is no longer in the list, so those same 16 bytes belong to the user. The allocator only reads the list pointers of a block whose free flag is set. `size` and `prevsize` are outside the union and are always valid.

### Free list

An intrusive doubly linked list: the nodes are the free blocks themselves, so the allocator never calls `malloc` for its own bookkeeping. New free blocks are inserted at the front. A block can be removed in O(1) because it knows both neighbours in the list.

### Allocation (`HmmAlloc`)

1. Return `NULL` for size 0 or for sizes that would overflow when rounded.
2. Round the size up to a multiple of 16 (once).
3. Search the free list for the smallest block that fits (best fit).
4. If none fits, grow the heap (`heapgrow`).
5. Remove the block from the free list and split off the unused tail if the leftover can hold a header plus 16 data bytes.
6. Return `block + 16`.

### Splitting

If a block is larger than needed and the leftover can hold its own 16-byte header plus at least 16 data bytes, the block is cut in two. The leftover becomes a new free block, the `prevsize` of the block after it is updated (or `lastheapblk` if the leftover is the last block). Otherwise the user keeps the whole block.

### Free (`HmmFree`)

1. Ignore `NULL` and blocks already marked free.
2. Insert the block into the free list and mark it free.
3. Merge with the next block in memory if it is free (`mergeblk`).
4. If the previous block (found with `prevsize`) is free, merge the previous block with this one. This reuses the same "merge with next" routine, started from the previous block, so the invariant "no two free blocks are adjacent" always holds.
5. Try to shrink the heap (`heapshrink`).

### Heap growth

The first call pads the start of the heap so it is 16-byte aligned. Each growth asks `sbrk` for at least 128 KB (more if the request is larger), so the number of system calls stays small. If `sbrk` fails, the allocation returns `NULL`. If the last block of the heap is free and adjacent to the new memory, the two are merged.

### Heap shrinking

After every `free`, `heapshrink` checks whether the last block in memory is free and at least 256 KB. If so, it keeps one 128 KB chunk and returns the rest to the OS with a negative `sbrk`. Keeping a chunk and using a threshold avoids moving the break up and down on every malloc/free pair. The break is only lowered if `sbrk(0)` still matches the allocator's recorded heap end, and the return value of `sbrk` is checked before any bookkeeping changes.

### Calloc and realloc

- `hmmCalloc` checks `nmemb * size` for overflow, allocates, and zeroes the block.
- `hmmReAlloc` handles `NULL` and size 0, returns the same pointer if the block is already large enough, otherwise allocates, copies and frees the old block. If the new allocation fails, the old block is left untouched.

## Build

Requires `gcc` and `make` (`sudo apt install build-essential`). Run all commands from the project folder (the one containing the `Makefile`).

```sh
cd heap_memory_manager
make              # static library, shared library and test executable
make static_lib   # build/lib/libhmm.a
make shared_lib   # build/lib/libhmm.so
make test         # build/hmm_test
make clean
```

## Tests

### Provided test

```sh
make test
./build/hmm_test
```

It allocates and frees small, large and huge blocks, checks that the program break is unchanged afterwards, validates data in 100-byte blocks, and then runs the realloc and calloc tests. Each stage prints `succeeded` when it passes.

### Program-break test (TLPI `free_and_sbrk`)

`test/free_and_sbrk.c` comes from the source code of *The Linux Programming Interface* (Michael Kerrisk). It checks whether `free` lowers the program break. `test/tlpi_hdr.h` is a minimal replacement for the book's header, so the book's source package is not needed.

```sh
make static_lib
gcc -Iinc -Itest test/free_and_sbrk.c build/lib/libhmm.a -o fas
./fas 1000 10240 1 500 1000
./fas 1000 10240 2 1 1000
./fas 1000 10240 1 1 999
```

Arguments are: number of allocations, block size, step, first block to free, last block to free.

| Command                         | Frees                                  | Expected break after `free()` | Observed  |
| ------------------------------- | -------------------------------------- | ------------------------------- | --------- |
| `./fas 1000 10240 1 500 1000` | blocks 500 to 1000 (a large free tail) | lowered                         | lowered   |
| `./fas 1000 10240 2 1 1000`   | every other block                      | unchanged                       | unchanged |
| `./fas 1000 10240 1 1 999`    | all but the last block                 | unchanged                       | unchanged |

In the last two cases the end of the heap is still in use, so there is no free tail to return.

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
- Assumes nothing else moves the program break between the allocator's `sbrk` calls. Growth merges and shrinking are skipped if the break is not where the allocator expects it.
- The double-free guard is best effort: it only works while the freed block's header is untouched.
- Best fit scans the whole free list, so allocation is O(n) in the number of free blocks.
- Only the free tail at the end of the heap can be returned to the OS; free blocks in the middle stay in the heap.
- Do not build with `-fsanitize`, since it conflicts with a replaced `malloc`.
