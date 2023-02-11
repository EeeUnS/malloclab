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
    "EUnS",
    /* First member's full name */
    "yunselee",
    /* First member's email address */
    "euns312510@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define W_SIZE 4 /* Word and header/footer size (bytes) */
#define D_SIZE 8 /* Double word size (bytes) */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))
/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(unsigned int *)(p))
#define PUT(p, val)     (*(unsigned int *)(p) = (val))

/* Read the size andallocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)
/* Given block ptr bp,*/
#define HEAD_P(bp)        ((char *)(bp) - W_SIZE)
#define FOOTER_P(bp)        ((char *)(bp) + GET_SIZE(HEAD_P(bp)) - D_SIZE)

#define NEXT_BLOCK_P(bp) ((char*)(bp) + GET_SIZE((char*)(bp) - W_SIZE))
#define PREV_BLOCK_P(bp) ((char*)(bp) - GET_SIZE((char*)(bp) - D_SIZE))

//declaration
static char* p_heap_list = NULL;
static void* extend_heap(size_t words);
static void* coalesce(void* bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);


int mm_init(void)
{
    /* Create the initial empty heap */
    if ((p_heap_list = mem_sbrk(4*W_SIZE)) == (void *)-1)
    {
        return -1;
    }
    PUT(p_heap_list, 0);                          /* Alignment padding */
    PUT(p_heap_list + (1*W_SIZE), PACK(D_SIZE, 1)); /* Prologue header */
    PUT(p_heap_list + (2*W_SIZE), PACK(D_SIZE, 1)); /* Prologue footer */
    PUT(p_heap_list + (3*W_SIZE), PACK(0, 1));     /* Epilogue header */
    p_heap_list += (2*W_SIZE);
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/W_SIZE) == NULL)
    {
        return -1;
    }

    return 0;
}

static void* extend_heap(size_t words) {
    
    char* bp;
    size_t size = (words % 2) ? (words + 1) * W_SIZE : (words) * W_SIZE;
    
    if ((long)(bp = mem_sbrk(size)) == -1) 
    {
        return NULL;
    }
    
    PUT(HEAD_P(bp), PACK(size, 0));
    PUT(FOOTER_P(bp), PACK(size, 0));
    PUT(HEAD_P(NEXT_BLOCK_P(bp)), PACK(0, 1));


    return coalesce(bp);
}

static void *coalesce (void *bp)
{
    size_t prev_alloc = GET_ALLOC(FOOTER_P (PREV_BLOCK_P (bp)));
    size_t next_alloc = GET_ALLOC (HEAD_P (NEXT_BLOCK_P (bp)));
    size_t size = GET_SIZE(HEAD_P (bp));

    if (prev_alloc && next_alloc) 
    {
        return bp;
    }

    else if (prev_alloc && !next_alloc) 
    {
        size += GET_SIZE (HEAD_P (NEXT_BLOCK_P (bp)));
        PUT (HEAD_P (bp), PACK(size, 0));
        PUT (FOOTER_P (bp), PACK (size,0));
 
    }

    else if (!prev_alloc && next_alloc) 
    {
        size += GET_SIZE (HEAD_P (PREV_BLOCK_P (bp)));
        PUT (FOOTER_P (bp), PACK(size, 0));
        PUT (HEAD_P (PREV_BLOCK_P (bp)), PACK(size, 0));
        bp = PREV_BLOCK_P (bp);
    }
    else 
    {
        size += GET_SIZE(HEAD_P (PREV_BLOCK_P (bp))) +
        GET_SIZE(FOOTER_P (NEXT_BLOCK_P (bp)));
        PUT (HEAD_P (PREV_BLOCK_P (bp)), PACK(size, 0));
        PUT (FOOTER_P (NEXT_BLOCK_P (bp)), PACK(size, 0));
        bp =PREV_BLOCK_P (bp);
    }
    return bp;
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;      /* Adjusted block size */
    char *bp;          /* Ignore spurious requests */ 

    if (size == 0)
    {
        return NULL;
    }
    if (size <= D_SIZE)
    {
        asize = 2*D_SIZE;
    }
    else
    {
        asize = D_SIZE * ((size + (D_SIZE) + (D_SIZE-1)) / D_SIZE);
    }

    if ((bp = find_fit (asize)) != NULL) 
    {
        place(bp, asize);
        return bp;  
    }

    size_t extendsize; /* Amount to extend heap if no fit */ 
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap (extendsize/W_SIZE)) == NULL)
    {
        return NULL;
    }
    place (bp, asize);
    return bp;
}

void *find_fit(size_t a_size) {
    char *bp = p_heap_list;
    bp = NEXT_BLOCK_P(bp);
    while (GET_SIZE(HEAD_P(bp)) < a_size || GET_ALLOC(HEAD_P(bp)) == 1) {
        bp = NEXT_BLOCK_P(bp);

        if (GET_SIZE(HEAD_P(bp)) == 0) 
        {
            return NULL;
        }
    }
    return bp;
}

static void place(void *bp, size_t a_size) {
    size_t c_size = GET_SIZE(HEAD_P(bp));

    if ((c_size - a_size) >= (2 * (D_SIZE))) 
    {
        // 요청 용량 만큼 블록 배치
        PUT(HEAD_P(bp), PACK(a_size, 1));
        PUT(FOOTER_P(bp), PACK(a_size, 1));
        
        bp = NEXT_BLOCK_P(bp);
        // 남은 블록에 header, footer 배치
        PUT(HEAD_P(bp), PACK(c_size - a_size, 0));
        PUT(FOOTER_P(bp), PACK(c_size - a_size, 0));
    }
    else 
    {// csize와 aszie 차이가 네 칸(16byte)보다 작다면 해당 블록 통째로 사용
        PUT(HEAD_P(bp), PACK(c_size, 1));
        PUT(FOOTER_P(bp), PACK(c_size, 1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HEAD_P(bp));

    PUT(HEAD_P(bp), PACK(size, 0));
    PUT(FOOTER_P(bp), PACK(size, 0));

    coalesce(bp);
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    if(size <= 0){ 
        mm_free(bp);
        return 0;
    }
    if(bp == NULL){
        return mm_malloc(size); 
    }
    void *ptr = mm_malloc(size); 
    if(ptr == NULL){
        return 0;
    }
    size_t oldsize = GET_SIZE(HEAD_P(bp));
    if(size < oldsize){
    	oldsize = size; 
	}
    memcpy(ptr, bp, oldsize); 
    mm_free(bp);
    return ptr;
}














