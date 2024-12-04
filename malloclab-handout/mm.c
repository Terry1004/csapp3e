/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "team",
    /* First member's full name */
    "Terry",
    /* First member's email address */
    "hym961004@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* enable/disable debug logs */
// #define DEBUG

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
/* amount of heap memory to extend */
#define PAGE_SIZE (1 << 10)
/* rounds up to the nearest multiple of page size */
#define ROUND_PAGE(size) (((size) / PAGE_SIZE + 1) * PAGE_SIZE)
/* minimum between 2 variables */
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define DSIZE 8         /* Double word size (bytes) */
#define NUM_FL 8        /* Number of segregated free lists */
#define MIN_FREE_SIZE 5 /* Minimum free block size in the power of 2 */

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))
/* Read/write a word at address p */
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))
/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
/* Given block pointer ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - DSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - 2 * DSIZE)
/* Given block pointer ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - 2 * DSIZE)))
/* Prologue block memory address */
#define PROLOGUE_BLKPTR ((char *)mem_heap_lo() + (3 * NUM_FL + 1) * DSIZE)
/* Epilogue block memory address: cannot dereference */
#define EPILOGUE_BLKPTR ((char *)mem_heap_hi() + 1)

/*
Segreated free list macros
Each list consists of blocks of size between 2^n and 2^(n+1) - 1
*/
/* Segregated free list prologue address */
#define PROLOGUE_FL(n) ((char *)mem_heap_lo() + (n - MIN_FREE_SIZE) * 3 * DSIZE)
/* Segregated free list epilogue address */
#define EPILOGUE_FL(n) ((char *)mem_heap_lo() + ((n - MIN_FREE_SIZE) * 3 + 2) * DSIZE)
/* Pointer operations on free lists
Each list consists of two pointers:
1. The first pointer points to the starting address of the previous free list
2. The second pointer points to the starting address of the next free list
*/
#define NEXT_PTR(bp) ((char *)(bp) + DSIZE)
#define PREV_FREE_BLK(bp) (*((void **)(bp)))
#define NEXT_FREE_BLK(bp) (*(void **)(NEXT_PTR(bp)))
#define PUT_FL(p, val) (*(void **)(p) = (void **)(val))

/* Optimization: track the last free block separately
because its capacity is theoretically unlimited
*/
static void *LAST_FREE_BP = NULL;

static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void *place(void *bp, size_t size, bool with_offset);
static int compute_free_list_index(size_t size);
static void add_to_free_list(void *bp, size_t size);
static void remove_from_free_list(void *bp);
static int mm_check(void);
static void dump_heap(void);
static void dump_fl(int n);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    void *heap_listp;
    heap_listp = mem_sbrk((3 * NUM_FL + 3) * DSIZE);
    if (heap_listp == (void *)-1)
        return -1;

    for (int i = 0; i < NUM_FL; i++)
    {
        /* i-th free list prologue */
        PUT_FL((char *)heap_listp + 3 * i * DSIZE, NULL);
        /* i-th free list prologue next pointer */
        PUT_FL((char *)heap_listp + (3 * i + 1) * DSIZE, (char *)heap_listp + (3 * i + 2) * DSIZE);
        /* i-th free list epilogue */
        PUT_FL((char *)heap_listp + (3 * i + 2) * DSIZE, (char *)heap_listp + 3 * i * DSIZE);
    }

    PUT((char *)heap_listp + 3 * NUM_FL * DSIZE, PACK(2 * DSIZE, 1));       /* prologue header */
    PUT((char *)heap_listp + (3 * NUM_FL + 1) * DSIZE, PACK(2 * DSIZE, 1)); /* prologue footer */
    PUT((char *)heap_listp + (3 * NUM_FL + 2) * DSIZE, PACK(0, 1));         /* epilogue header */

    /* extend the empty heap with a free page */
    if (extend_heap(PAGE_SIZE) == NULL)
        return -1;

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    void *bp;
    /* ignore spurious requests */
    if (size == 0)
        return NULL;

    size = ALIGN(size + 2 * DSIZE);

    /* search the free list for a fit */
    if ((bp = find_fit(size)) == NULL)
        return NULL;

    remove_from_free_list(bp);
    bp = place(bp, size, true);

#ifdef DEBUG
    printf("[after alloc]\n");
    dump_heap();
#endif
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    add_to_free_list(bp, size);
    coalesce(bp);

#ifdef DEBUG
    printf("[after free]\n");
    dump_heap();
#endif
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    void *new_bp, *next_bp;
    size_t old_size, copy_size, next_size, extend_size;

    /* handle spurious case */
    if (bp == NULL)
        return mm_malloc(size);
    if (size == 0)
    {
        mm_free(bp);
#ifdef DEBUG
        printf("[after realloc]\n");
        dump_heap();
#endif
        return NULL;
    }

    old_size = GET_SIZE(HDRP(bp));
    size = ALIGN(size + 2 * DSIZE);

    if (size <= old_size)
    {
        bp = place(bp, size, false);
#ifdef DEBUG
        printf("[after realloc]\n");
        dump_heap();
#endif
        return bp;
    }

    next_bp = NEXT_BLKP(bp);
    next_size = GET_SIZE(HDRP(next_bp));
    if (!GET_ALLOC(HDRP(next_bp)) && size <= old_size + next_size)
    {
        remove_from_free_list(next_bp);
        PUT(HDRP(bp), PACK(old_size + next_size, 0));
        PUT(FTRP(bp), PACK(old_size + next_size, 0));
        bp = place(bp, size, false);
#ifdef DEBUG
        printf("[after realloc]\n");
        dump_heap();
#endif
        return bp;
    }

    if (next_bp == (void *)EPILOGUE_BLKPTR || next_bp == LAST_FREE_BP)
    {
        if (LAST_FREE_BP != NULL)
            extend_size = size - old_size - GET_SIZE(HDRP(LAST_FREE_BP));
        else
            extend_size = size - old_size;

        if ((next_bp = extend_heap(ROUND_PAGE(extend_size))) != NULL)
        {
            remove_from_free_list(next_bp);
            next_size = GET_SIZE(HDRP(next_bp));
            PUT(HDRP(bp), PACK(old_size + next_size, 0));
            PUT(FTRP(bp), PACK(old_size + next_size, 0));
            bp = place(bp, size, false);
#ifdef DEBUG
            printf("[after realloc]\n");
            dump_heap();
#endif
            return bp;
        }
    }

    copy_size = MIN(old_size, size) - 2 * DSIZE;
    char overwritten[2 * DSIZE]; /* The first 16 bytes will be overwritte by mm_free */
    memcpy(overwritten, bp, 2 * DSIZE);
    mm_free(bp);

    /* search the free list for a fit */
    if ((new_bp = find_fit(size)) == NULL)
        return NULL;

    remove_from_free_list(new_bp);
    memmove(new_bp, bp, copy_size);
    memcpy(new_bp, overwritten, 2 * DSIZE);
    new_bp = place(new_bp, size, true);

#ifdef DEBUG
    printf("[after realloc]\n");
    dump_heap();
#endif
    return new_bp;
}

// void *mm_realloc(void *bp, size_t size)
// {
//     void *new_bp;
//     size_t old_size, copy_size;

//     /* handle spurious case */
//     if (bp == NULL)
//         return mm_malloc(size);

//     old_size = GET_SIZE(HDRP(bp));
//     size = ALIGN(size + 2 * DSIZE);
//     copy_size = MIN(old_size, size) - 2 * DSIZE;

//     char overwritten[2 * DSIZE]; /* The first 16 bytes will be overwritte by mm_free */
//     memcpy(overwritten, bp, 2 * DSIZE);
//     mm_free(bp);
//     if (size == 0)
//         return NULL;

//     /* search the free list for a fit */
//     if ((new_bp = find_fit(size)) == NULL)
//         return NULL;

//     remove_from_free_list(new_bp);
//     memmove(new_bp, bp, copy_size);
//     memcpy(new_bp, overwritten, 2 * DSIZE);
//     place(new_bp, size);

// #ifdef DEBUG
//     printf("[after realloc]\n");
//     dump_heap();
// #endif
//     return new_bp;
// }

static void *extend_heap(size_t size)
{
    char *bp;

    size = ALIGN(size);
    if ((bp = mem_sbrk(size)) == (void *)-1)
        return NULL;

    if (LAST_FREE_BP != NULL)
    {
        bp = (char *)LAST_FREE_BP;
        size += GET_SIZE(HDRP(LAST_FREE_BP));
    }

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* new epilogue header */

    /* Add the block to free list */
    add_to_free_list(bp, size);

    return bp;
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
        return bp;
    else if (prev_alloc && !next_alloc)
    {
        remove_from_free_list(bp);
        remove_from_free_list(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        add_to_free_list(bp, size);
    }
    else if (!prev_alloc && next_alloc)
    {
        remove_from_free_list(bp);
        remove_from_free_list(PREV_BLKP(bp));

        bp = PREV_BLKP(bp);
        size += GET_SIZE(FTRP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        add_to_free_list(bp, size);
    }
    else
    {
        remove_from_free_list(bp);
        remove_from_free_list(PREV_BLKP(bp));
        remove_from_free_list(NEXT_BLKP(bp));

        size += GET_SIZE(FTRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        add_to_free_list(bp, size);
    }

    return bp;
}

static void *find_fit(size_t size)
{
    int n;
    void *new_bp;
    size_t last_free_size;

    for (n = compute_free_list_index(size); n < MIN_FREE_SIZE + NUM_FL; n++)
    {
        void *prologue = PROLOGUE_FL(n);
        void *epilogue = EPILOGUE_FL(n);
        void *bp = NEXT_FREE_BLK(prologue);
        while (bp != epilogue)
        {
            if (GET_SIZE(HDRP(bp)) >= size)
                return bp;
            bp = NEXT_FREE_BLK(bp);
        }
    }

    if (LAST_FREE_BP != NULL)
    {
        last_free_size = GET_SIZE(HDRP(LAST_FREE_BP));
        if (last_free_size >= size)
            return LAST_FREE_BP;
        size -= last_free_size;
    }

    if ((new_bp = extend_heap(ROUND_PAGE(size))) == NULL)
        return NULL;

    return new_bp;
}

static void *place(void *bp, size_t size, bool with_offset)
{
    size_t curr_size = GET_SIZE(HDRP(bp));
    size_t offset, remainder;
    void *remainder_bp;

    offset = MIN(curr_size - size, size);
    if (with_offset && offset > (1 << MIN_FREE_SIZE))
    {
        PUT(HDRP(bp), PACK(offset, 0));
        PUT(FTRP(bp), PACK(offset, 0));
        add_to_free_list(bp, offset);
        bp = NEXT_BLKP(bp);
    }
    else
        offset = 0;

    remainder = curr_size - size - offset;
    if (remainder < (1 << MIN_FREE_SIZE))
    {
        PUT(HDRP(bp), PACK(curr_size - offset, 1));
        PUT(FTRP(bp), PACK(curr_size - offset, 1));
        return bp;
    }

    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
    remainder_bp = NEXT_BLKP(bp);
    PUT(HDRP(remainder_bp), PACK(remainder, 0));
    PUT(FTRP(remainder_bp), PACK(remainder, 0));
    add_to_free_list(remainder_bp, remainder);
    return bp;
}

static int compute_free_list_index(size_t size)
{
    int n;
    for (n = MIN_FREE_SIZE + 1; n < MIN_FREE_SIZE + NUM_FL; n++)
    {
        if (size >> n == 0)
            return n - 1;
    }
    return MIN_FREE_SIZE + NUM_FL - 1;
}

static void add_to_free_list(void *bp, size_t size)
{
    if (NEXT_BLKP(bp) == EPILOGUE_BLKPTR)
    {
        LAST_FREE_BP = bp;
        return;
    }

    int n = compute_free_list_index(size);
    void *prologue = PROLOGUE_FL(n);
    void *next = NEXT_FREE_BLK(prologue);

    PUT_FL(NEXT_PTR(prologue), bp);
    PUT_FL(next, bp);
    PUT_FL(bp, prologue);
    PUT_FL(NEXT_PTR(bp), next);
}

static void remove_from_free_list(void *bp)
{
    if (bp == LAST_FREE_BP)
    {
        LAST_FREE_BP = NULL;
        return;
    }

    void *prev = PREV_FREE_BLK(bp);
    void *next = NEXT_FREE_BLK(bp);
    PUT_FL(NEXT_PTR(prev), next);
    PUT_FL(next, prev);
}

static int mm_check(void)
{
    void *curr = PROLOGUE_BLKPTR;
    while (curr < (void *)EPILOGUE_BLKPTR)
    {
        size_t header_size = GET_SIZE(HDRP(curr));
        size_t header_alloc = GET_ALLOC(HDRP(curr));
        size_t footer_size = GET_SIZE(FTRP(curr));
        size_t footer_alloc = GET_ALLOC(FTRP(curr));

        if (header_size != footer_size || header_alloc != footer_alloc)
        {
            printf("header and footer are unequal\n");
            dump_heap();
            return 1;
        }
        curr = NEXT_BLKP(curr);
    }

    for (int n = MIN_FREE_SIZE; n < MIN_FREE_SIZE + NUM_FL; n++)
    {
        curr = NEXT_FREE_BLK(PROLOGUE_FL(n));
        while (curr != EPILOGUE_FL(n))
        {
            size_t header_size = GET_SIZE(HDRP(curr));
            size_t header_alloc = GET_ALLOC(HDRP(curr));
            size_t footer_size = GET_SIZE(FTRP(curr));
            size_t footer_alloc = GET_ALLOC(FTRP(curr));

            if (header_size != footer_size || header_alloc != footer_alloc)
            {
                printf("header and footer are unequal in segregated list [%d]\n", n);
                dump_fl(n);
                return 1;
            }
            if (header_alloc != 0)
            {
                printf("block is allocated in segregated list [%d]\n", n);
                dump_fl(n);
                return 1;
            }
            curr = NEXT_FREE_BLK(curr);
        }
    }
    return 0;
}

static void dump_heap(void)
{
    void *curr = NEXT_BLKP(PROLOGUE_BLKPTR);
    while (curr < (void *)EPILOGUE_BLKPTR)
    {
        size_t header_size = GET_SIZE(HDRP(curr));
        size_t header_alloc = GET_ALLOC(HDRP(curr));
        size_t footer_size = GET_SIZE(FTRP(curr));
        size_t footer_alloc = GET_ALLOC(FTRP(curr));

        printf("header=(size=%ld, alloc=%ld), footer=(size=%ld, alloc=%ld)\n", header_size, header_alloc, footer_size, footer_alloc);
        if (header_size != footer_size || header_alloc != footer_alloc)
        {
            return;
        }
        curr = NEXT_BLKP(curr);
    }
}

static void dump_fl(int n)
{
    void *curr = NEXT_FREE_BLK(PROLOGUE_FL(n));
    while (curr != EPILOGUE_FL(n))
    {
        size_t header_size = GET_SIZE(HDRP(curr));
        size_t header_alloc = GET_ALLOC(HDRP(curr));
        size_t footer_size = GET_SIZE(FTRP(curr));
        size_t footer_alloc = GET_ALLOC(FTRP(curr));

        printf("header=(size=%ld, alloc=%ld), footer=(size=%ld, alloc=%ld)\n", header_size, header_alloc, footer_size, footer_alloc);
        if (header_size != footer_size || header_alloc != footer_alloc)
        {
            return;
        }
        curr = NEXT_FREE_BLK(curr);
    }
}
