#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SMALL_BLOCKS_ALLOCS_COUNT (1024)
#define SMALL_BLOCKS_SIZE         (90 - sizeof(size_t))
#define LARGE_BLOCKS_ALLOCS_COUNT (100)
#define LARGE_BLOCKS_SIZE         (1024 - sizeof(size_t))
#define HUGE_BLOCKS_ALLOCS_COUNT  (2)
#define HUGE_BLOCKS_SIZE          ((50 * 1024) - sizeof(size_t))

#define VALIDATION_WORD           (0xFF55FF55)
#define ARRAY_SIZE                (100)
#define ARRAY_ELEMENTS_COUNT      (ARRAY_SIZE / sizeof(uint32_t))

/* Remove these macros if realloc or calloc is not defined. */
#define REALLOC_IS_DEFINED
#define CALLOC_IS_DEFINED

extern char end, edata, etext;

void memset_word(void *ptr, unsigned int value, size_t num_words);

int main(void)
{
  char *heapEnd;
  int *ptr[SMALL_BLOCKS_ALLOCS_COUNT];

  printf("etext = %p, edata=%p, end=%p, initial program break=%p\n",
         &etext, &edata, &end, (char *)sbrk(0));

  heapEnd = (char *)sbrk(0);

  for (int index = 0; index < SMALL_BLOCKS_ALLOCS_COUNT; index++)
  {
    ptr[index] = malloc(SMALL_BLOCKS_SIZE);
    if (ptr[index] == NULL)
    {
      printf("failed to allocate ptr[%d]\n", index);
      return EXIT_FAILURE;
    }
  }

  for (int index = 0; index < SMALL_BLOCKS_ALLOCS_COUNT; index++)
  {
    free(ptr[index]);
  }

  for (int index = 0; index < LARGE_BLOCKS_ALLOCS_COUNT; index++)
  {
    ptr[index] = malloc(LARGE_BLOCKS_SIZE);
    if (ptr[index] == NULL)
    {
      printf("failed to allocate ptr[%d]\n", index);
      return EXIT_FAILURE;
    }
  }

  for (int index = 0; index < LARGE_BLOCKS_ALLOCS_COUNT; index++)
  {
    free(ptr[index]);
  }

  for (int index = 0; index < HUGE_BLOCKS_ALLOCS_COUNT; index++)
  {
    ptr[index] = malloc(HUGE_BLOCKS_SIZE);
    if (ptr[index] == NULL)
    {
      printf("failed to allocate ptr[%d]\n", index);
      return EXIT_FAILURE;
    }
  }

  for (int index = 0; index < HUGE_BLOCKS_ALLOCS_COUNT; index++)
  {
    free(ptr[index]);
  }

  if (heapEnd != (char *)sbrk(0))
  {
    printf("First test: failed, the old heap end = %p and the new end = %p\n",
           heapEnd, (char *)sbrk(0));
  }
  else
  {
    printf("First test: succeeded\n");
  }

  for (int index = 0; index < LARGE_BLOCKS_ALLOCS_COUNT; index++)
  {
    ptr[index] = malloc(ARRAY_SIZE);
    if (ptr[index] == NULL)
    {
      printf("failed to allocate ptr[%d]\n", index);
      return EXIT_FAILURE;
    }

    memset_word(ptr[index], VALIDATION_WORD, ARRAY_ELEMENTS_COUNT);
  }

  for (int index = 0; index < LARGE_BLOCKS_ALLOCS_COUNT; index++)
  {
    assert(ptr[index][0] == VALIDATION_WORD);
    assert(ptr[index][ARRAY_ELEMENTS_COUNT - 1] == VALIDATION_WORD);
    assert(ptr[index][ARRAY_ELEMENTS_COUNT / 2] == VALIDATION_WORD);
    free(ptr[index]);
  }

  printf("Second test: succeeded\n");

  for (int pass = 0; pass < 2; pass++)
  {
    for (int index = 0; index < LARGE_BLOCKS_ALLOCS_COUNT; index++)
    {
      ptr[index] = malloc(5);
      if (ptr[index] == NULL)
      {
        printf("failed to allocate ptr[%d]\n", index);
        return EXIT_FAILURE;
      }

      memset(ptr[index], 0xff, 5);
    }

    for (int index = 0; index < LARGE_BLOCKS_ALLOCS_COUNT; index++)
    {
      free(ptr[index]);
    }
  }

  printf("Third test: succeeded\n");

#ifdef REALLOC_IS_DEFINED
  int *allocatedPtr = malloc(4 * sizeof(uint32_t));

  allocatedPtr[0] = VALIDATION_WORD;
  allocatedPtr[1] = VALIDATION_WORD;
  allocatedPtr[2] = VALIDATION_WORD;
  allocatedPtr[3] = VALIDATION_WORD;

  allocatedPtr = realloc(allocatedPtr, 200);

  assert(allocatedPtr[0] == VALIDATION_WORD);
  assert(allocatedPtr[1] == VALIDATION_WORD);
  assert(allocatedPtr[2] == VALIDATION_WORD);
  assert(allocatedPtr[3] == VALIDATION_WORD);

  free(allocatedPtr);
  printf("REALLOC test: succeeded\n");
#endif /* REALLOC_IS_DEFINED */

#ifdef CALLOC_IS_DEFINED
  uint32_t *heapArray = calloc(20, 4);

  heapArray[2] = VALIDATION_WORD;
  heapArray[3] = VALIDATION_WORD;

  assert(heapArray[0] == 0);
  assert(heapArray[1] == 0);
  assert(heapArray[2] == VALIDATION_WORD);
  assert(heapArray[3] == VALIDATION_WORD);

  free(heapArray);
  printf("CALLOC test: succeeded\n");
#endif /* CALLOC_IS_DEFINED */

  return EXIT_SUCCESS;
}

void memset_word(void *ptr, uint32_t value, size_t num_words)
{
  uint32_t *current = (uint32_t *)ptr;

  while (num_words != 0U)
  {
    *current = value;
    current++;
    num_words--;
  }
}