/**
 *  @file   memmanager.c
 *
 * @brief   Memory Allocation for C
 *
 *  @note   Mentioned in Memory Allocation in C in Embedded Systems Programming, Aug. 89.
 *          https://www.embedded.com/memory-allocation-in-c/
 *
 *  @note   Original produced by Programming ARTS (8/18/88)
 *          Programmers: Les Aldridge, Travis I. Seay
 *
 *  @note   Updated to C89/C99
 *          Added doxygen documentation
 *          Name changed
 *              free        ->      MemFree
 *              malloc      ->      MemAlloc
 *              i_alloc     ->      MemInit
 *          All static variables are now initialized
 *          Add regions for allocation
 *
 *  @note   Header could be smaller for used blocks because the next pointer is only
 *          used for free blocks.
 *
 *  @author Les Aldridge, Travis I. Seay (original version)
 *
 *  @author Hans Schneebeli (updated version)
 */

#include <stdint.h>

#include "memmanager.h"

/**
 *  @brief  NULL Pointer
 *
 *  @note   To avoid use of additional headers. This will be used on embedded systems
 */
///@{
#ifndef NULL
#define NULL (0)
#endif
///@}

/// Include stdio.h only for testing or debugging
#if defined(DEBUG) || defined(TEST)
#include <stdio.h>
#endif

/**
 *  @brief  header structure for each block
 *
 *  @note   next points to the next free block, otherwise it is NULL
 *
 *  @note   Using bit fields. It will later be used in an embedded system
 *
 *  @note   Assumed unsigned it is 32 bits long
 */
typedef struct header {
    union {
        uint32_t    word;
        struct {
            uint32_t    used:1;         ///< 1 bit for used/free flag
            uint32_t    region:2;       ///< 2 bits for region
            uint32_t    size:29;        ///< 29 bits for size (=512 MBytes)
        };
    };
    union {
        struct header  *next;           ///< Next free block
        uint32_t        area[1];        ///< Place marker
    };
} HEADER;

/**
 *  @brief  Region definition
 *
 *  @note   Definition of a heap area
 */

typedef struct region {
    HEADER  *start;                     ///< Start address of this heap
    HEADER  *end;                       ///< End address of this heap
    HEADER  *free;                      ///< Pointer to first free block (Free list)
    int32_t  memleft;                   ///< Free area in sizeof(HEADER) units
} REGION;

/**
 *  @brief  Region definition
 *
 *  @note   Heap information loaded by MemInit
 *
 *  @note   To change the number of regions, the region field in HEADER must be changed too
 */
static REGION Regions[4] = {
    { .start = 0, .end = 0 },
    { .start = 0, .end = 0 },
    { .start = 0, .end = 0 },
    { .start = 0, .end = 0 }
};


/**
 *  @brief  Add a region to the pool
 *
 *  @note   Area must be aligned to an uint32_t
 */
void
MemAddRegion( uint32_t region, void *area, uint32_t size) {
REGION *r;

    r = &Regions[region];

    // If already initialized, do nothing
    if( r->start )
        return;

    r->start = area;
    r->end   = (HEADER *)((char *) area + size);
    r->free  = area;
    r->free->next = NULL;
    r->free->size = size/sizeof(HEADER)-1;
    r->free->used = 0;
    r->memleft = r->free->size;
}


/**
 *  @brief  MemInit
 *
 *  @note   Initializes heap info
 *
 *  @note   There are two versions. A version without parameters, that uses
 *          symbols defined by the linker. Another one, with explicit parameters.
 *          The preprocessor symbol MEM_LINKERINIT chooses one
 */
#ifdef MEM_LINKERINIT
void MemInit(void) {

uint32_t size = (char *) &_heapend - char *) &_heapstart;

    MemAddRegion( 0, &_heapstart, size);

}
#else
void MemInit(void *area, uint32_t size) {

    MemAddRegion( 0, area, size);

}
#endif


/**
 *  @brief  MemFree
 *
 *  @note   Return memory to free list.
 *          Where possible, make contiguous blocks of free memory.
 *          Assumes that 0 is not a valid address for allocation. Also,
 *          MemInit() must be called prior to using either MemFree() or MemAlloc();
 *
 *  @note   The Free List is kept in crescent order of address
 *
 *  @note   There are 4 case to consider:
 *
 *  Previous |  Next  | Action
 *  ---------|--------|--------------------------
 *   Busy    |  Busy  | Just add this block to free list
 *   Free    |  Busy  | Add size to the previous block
 *   Busy    |  Free  | Add size of the next block, add this block to list
 *   Free    |  Free  | Add size of the next block and combine both to previous one
 *
 *  Attention: Do not forget the remove the combined blocks from free list
 *
 *  There are the limit cases to consider, start and end of area
 */
void MemFree(void *p) {
HEADER *block, *prev, *f, *old, *nxt;
REGION *r;

    if( !p )
        return;

    f = (HEADER *)p - 1;                /* Point to header of block being returned. */
#ifdef DEBUG
    printf("Freeing element at %p with %d elements and area at %p\n",f,f->size,p);
#endif

    // Already free
    if( !f->used )
        return;

    // Get region used for allocation
    r = &Regions[f->region];

    r->memleft += f->size;

    /*
     * The Free list in kept in crescent order of address.
     *
     * Free-space head is higher up in memory than returnee. The returnee will
     * be the new head
     */
    if (f < r->free ) {
        old = r->free;                    /* Old head */
        r->free = f;                        /* New head */
        /* The only possibility is that the old head points to a contiguos block*/
        nxt = f + f->size;                 /* Right after new head */

        if (nxt == old) {                /* Old and new are contiguous. */
            f->size += old->size;         /* Combine them    */
            f->next = old->next;          /* forming one block. */
        } else {
            f->next = old;
        }
        f->used = 0;
        return;
    }

    /*
     * Otherwise, current free-space head is lower in memory. Walk down
     * free-space list looking for the block being returned. If the next pointer
     * points past the block, make a new entry and link it. If next pointer plus
     * its size points to the block, form one contiguous block.
     */

    block = r->free;
    prev = NULL;
    while ( block && f > block  ) {
        if (block+block->size == f) {
            block->size += f->size;     /* They're contiguous. */
            f = block + block->size;     /* Form one block. */
            if (f==block->next) {
                /*
                 * The new, larger block is contiguous to the next free block,
                 * so form a larger block. There's no need to continue this checking
                 * since if the block following this free one
                 * were free, the two would already have been combined.
                 */
                block->size += f->size;
                block->next = f->next;
                block->used = 0;
            }
            return;
        }
        prev=block;
        block=block->next;
    }

    /*
     * The address of the block being returned is greater than one in
     * the free queue (block) or the end of the queue was reached.
     * If at end, just link to the end of the queue.
     * Therefore, block is null or points to a block higher up in memory
     * than the one being returned.
     */
    prev->next = f;                 /* link to queue */
    prev = f + f->size;             /* right after space to free */
    if (prev == block) {            /* 'f' and 'block' are contiguous. */
        f->size += block->size;
        f->next = block->next;         /* Form a larger, contiguous block. */
    } else {
        f->next = block;
    }
    f->used = 0;
    return;
}


/**
 *  @brief  MemAlloc
 *
 *  @note   Returns a pointer to an allocate memory block if found.
 *          Otherwise, returns NULL
 *
 *  @note   It uses a first fit algorithm
 *
 *  @note   Allocate the space requested plus space for the header of the block.
 *          Search the free-space queue for a block that's large enough.
 *          If block is larger than needed, break into two pieces
 *          and allocate the portion higher up in memory.
 *          Otherwise, just allocate the entire block.
 *
 */
void *MemAlloc(uint32_t nb, uint32_t region) {
HEADER *block, *prev;
REGION *r;
uint32_t    nelems;

    /* Round to a multiple of sizeof(HEADER) */
    nelems = (nb+sizeof(HEADER)-1)/sizeof(HEADER) + 1;

#ifdef DEBUG
    printf("Allocating %u bytes (=%u elements)\n",nb,nelems);
#endif

    r = &Regions[region];

    for (prev=NULL,block=r->free; block!=NULL; block = block->next) {
        /* First fit */
        if ( nelems <= block->size ) {        /* Big enough */
            if ( nelems < block->size ) {
                block->size -= nelems;         /* Allocate tell end. */
                block->used = 0;
                block->next = NULL;         /* Mark as occupied */
                block += block->size;
                block->size = nelems;         /* block now == pointer to be alloc'd. */
                block->used = 1;
            } else {
                if (prev==NULL) {
                    r->free = block->next;
                } else {
                    prev->next = block->next;
                }
            }
            r->memleft -= nelems;

            /*
             * Return a pointer past the header to the actual space requested.
             */
            return((void *)(block+1));
        }
    }

    /* Area not found */
    return NULL;
}


/**
 *  @brief  MemStats
 *
 *  @note   Delivers allocation information
 */
void MemStats( MEMSTATS *stats, uint32_t region ) {
REGION *r;
HEADER *p;
const uint32_t MAXBYTES = 1000000;   /* to avoid the inclusion of other headers */

    r = &Regions[region];

    stats->memleft     = r->memleft;
    stats->freeblocks  = 0;
    stats->freebytes   = 0;
    stats->usedblocks  = 0;
    stats->usedbytes   = 0;
    stats->largestused = 0;
    stats->smallestused= MAXBYTES;
    stats->largestfree = 0;
    stats->smallestfree= MAXBYTES;

    if( !r->free )
        return;

    for(p=r->free;p;p=p->next) {
        stats->freeblocks++;
        stats->freebytes += p->size;
        if( p->size > stats->largestfree )
            stats->largestfree = p->size;
        if( p->size < stats->smallestfree )
            stats->smallestfree = p->size;
    }

    for(p=r->start;(p < r->end)&&(p->size>0);p=p+p->size) {
        if( p->used ) {
            stats->usedblocks++;
            stats->usedbytes += p->size;
            if( p->size > stats->largestused )
                stats->largestused = p->size;
            if( p->size < stats->smallestused )
                stats->smallestused = p->size;
        }
    }
    // To avoid "strange" numbers on output
    if( stats->smallestfree == MAXBYTES )
        stats->smallestfree = 0;
    if( stats->smallestused == MAXBYTES )
        stats->smallestused = 0;
    // To report sizes in bytes
    stats->freebytes    *= sizeof(HEADER);
    stats->usedbytes    *= sizeof(HEADER);
    stats->largestfree  *= sizeof(HEADER);
    stats->largestused  *= sizeof(HEADER);
    stats->smallestfree *= sizeof(HEADER);
    stats->smallestused *= sizeof(HEADER);
    stats->memleft      *= sizeof(HEADER);

}

#if defined(DEBUG) || defined(TEST)

/**
 *  @brief  Memory List
 *
 *  @note   List all memory blocks, including size and status
 *
 */
void MemList(uint32_t region) {
uint32_t i;
HEADER *p;
REGION *r;

    r = &Regions[region];

    for(i=0,p=r->start;(p<r->end)&&(p->size>0);i++,p=p+p->size) {
        printf("B%02u (%c): %u @%p (next=%p)\n",i,p->used?'U':'F',
                    (uint32_t) (p->size*sizeof(HEADER)),p,p->next);
    }
    putchar('\n');
}

#endif

//////////////////////// TEST  ///////////////////////////////////////////////////////////////////

#ifdef TEST
#include <stdio.h>

void PrintStats(char *msg, MEMSTATS *stats ) {

    puts(msg);
    printf("Free blocks      = %u\n",stats->freeblocks);
    printf("Free bytes       = %u\n",stats->freebytes);
    printf("Smallest free    = %u\n",stats->smallestfree);
    printf("Largest free     = %u\n",stats->largestfree);
    printf("Used blocks      = %u\n",stats->usedblocks);
    printf("Used bytes       = %u\n",stats->usedbytes);
    printf("Smallest used    = %u\n",stats->smallestused);
    printf("Largest used     = %u\n",stats->largestused);
    printf("Memory left      = %u\n",stats->memleft);

}

#define BUFFERSIZE 160

static uint32_t buffer[(BUFFERSIZE+sizeof(uint32_t)-1)/sizeof(uint32_t)];


int main(void) {
char *p1,*p2,*p3;
MEMSTATS stats;

    printf("Size of block HEADER = %u\n",(uint32_t) sizeof(HEADER));
    printf("Size of heap area    = %u\n",(uint32_t) BUFFERSIZE);

    MemInit(buffer,BUFFERSIZE);
    MemStats(&stats,0);
    PrintStats("Inicialized",&stats);
    MemList(0);

    p1 = MemAlloc(10,0);
    MemStats(&stats,0);
    PrintStats("Allocation #1",&stats);
    MemList(0);

    p2 = MemAlloc(10,0);
    MemStats(&stats,0);
    PrintStats("Allocation #2",&stats);
    MemList(0);

    p3 = MemAlloc(10,0);
    MemStats(&stats,0);
    PrintStats("Allocation #3",&stats);
    MemList(0);

    MemFree(p2);
    MemStats(&stats,0);
    PrintStats("Free #2",&stats);
    MemList(0);

    MemFree(p3);
    MemStats(&stats,0);
    PrintStats("Free #3",&stats);
    MemList(0);

    MemFree(p1);
    MemStats(&stats,0);
    PrintStats("Free #3",&stats);
    MemList(0);

}
#endif
