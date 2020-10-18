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
 *  @author Les Aldridge, Travis I. Seay (original version)
 *
 *  @author Hans Schneebeli (updated version)
 */

#ifndef NULL
#define NULL (void *)0
#endif

typedef struct hdr {
    struct hdr      *ptr;
    unsigned int    size;
} HEADER;


#ifdef MEM_LINKERINIT
/*  Symbols defined in the linker script
 * _heapstart and _heapend must be at the first and last byte of the heap, respectively */
extern HEADER _heapstart;       /*!< first byte allocated to the heap */
extern HEADER _heapend;         /*!< last byte allocated to the heap */
#endif

/*  Must be initialized in MemInit to point to first and last byte
 * of heap, respectively */
static HEADER *heapstart;       /*!< first byte allocated to the heap */
static HEADER *heapend;         /*!< last byte allocated to the heap */

/**
 *  @brief  Free list
 */
///@{}
static HEADER   *frhd;      /* pointer to free list */
static short    memleft;    /* memory left */
///@}

/**
 *  @brief  MemFree
 *
 *  @note   Return memory to free list.
 *          Where possible, make contiguous blocks of free memory.
 *          Assumes that 0 is not a valid address for allocation. Also,
 *          MemInit() must be called prior to using either MemFree() or MemAlloc();
 *          otherwise, the free list will be null.
 */

void MemFree(void *ap) {

    HEADER *nxt, *prev, *f;
    f = (HEADER *)ap - 1;   /* Point to header of block being returned. */
    memleft += f->size;

    /*  frhd is never null unless i_alloc() wasn't called to initialize package. */

    if (frhd > f) {

        /* Free-space head is higher up in memory than returnee. */

        nxt = frhd;     /* old head */
        frhd = f;   /* new head */
        prev = f + f->size;     /* right after new head */

        if (prev==nxt)  {   /* Old and new are contiguous. */
            f->size += nxt->size;
            f->ptr = nxt->ptr;  /* Form one block. */
        }
        else {
            f->ptr = nxt;
        }
    return;
    }

    /*  Otherwise, current free-space head is lower in memory. Walk down free-space list
     * looking for the block being returned. If the next pointer points past the block,
     * make a new entry and link it. If next pointer plus its size points to the block,
     * form one contiguous block. */

    nxt = frhd;
    for (nxt=frhd; nxt && nxt < f; prev=nxt,nxt=nxt->ptr) {
        if (nxt+nxt->size == f) {
            nxt->size += f->size;   /* They're contiguous. */
            f = nxt + nxt->size;    /* Form one block. */
            if (f==nxt->ptr) {
                /* The new, larger block is contiguous to the next free block, so form a
                 * larger block.There's no need to continue this checking since if the block
                 * following this free one were free, the two would already have been combined. */
                nxt->size += f->size;
                nxt->ptr = f->ptr;
            }
            return;
        }
    }

    /*  The address of the block being returned is greater than one in the free queue (nxt)
     * or the end of the queue was reached. If at end, just link to the end of the queue.
     * Therefore, nxt is null or points to a block higher up in memory than the one being
     * returned. */

    prev->ptr = f;              /* link to queue */
    prev = f + f->size;         /* right after space to free */
    if (prev == nxt) {          /* 'f' and 'nxt' are contiguous. */
        f->size += nxt->size;
        f->ptr = nxt->ptr;      /* Form a larger, contiguous block. */
    }
    else {
        f->ptr = nxt;
    }

    return;
}

/**
 *  @brief  MemAlloc
 *
 *  @note   Returns a pointer to an allocate memory block if found.
 *          Otherwise, returns NULL
 *
 *  @note
 *
 */

void *MemAlloc(int nbytes) {  /* bytes to allocate */
HEADER *nxt, *prev;
int         nunits;
nunits = (nbytes+sizeof(HEADER)-1) / sizeof(HEADER) + 1;

    /* Allocate the space requested plus space for the header of the block. Search the free-
     * space queue for a block that's large enough. If block is larger than needed, break into
     * two pieces and allocate the portion higher up in memory. Otherwise, just allocate the
     * entire block. */

    for (prev=NULL,nxt=frhd; nxt; nxt = nxt->ptr) {
        if (nxt->size >= nunits) {      /* big enough */
            if (nxt->size > nunits) {
                nxt->size -= nunits;    /* Allocate tell end. */
                nxt += nxt->size;
                nxt->size = nunits;     /* nxt now == pointer to be alloc'd. */
            } else {
                if (prev==NULL)
                    frhd = nxt->ptr;
                else
                    prev->ptr = nxt->ptr;
            }
            memleft -= nunits;

            /* Return a pointer past the header to the actual space requested. */
            return((char *)(nxt+1));
        }
    }

    return(NULL);
}

/**
 *  @brief  MemInit
 *
 *  @note   Initializes heap info
 *
 *  @note   There are two versions. A version without parameters, that uses
 *          symbols defined by the linker. Another one, with explicit parameters.
 *          The preprocessor symbol MEM_LINKERINIT chooses one
 *
 *  @note   The area must be word aligned
 */

#ifdef MEM_LINKERINIT
void MemInit(void) {

    /* Initialize the free list */
    frhd = &_heapstart;
    frhd->ptr = NULL;
    frhd->size = ((char *)&_heapend - (char *)&_heapstart) / sizeof(HEADER);
    memleft = frhd->size;   /* initial size in four-byte units */
}
#else
void MemInit(void *area, int size) {

    heapstart = (HEADER *) area;
    heapend   = (HEADER *) ((char *) area + size - 1);
    /* Initialize the free list */
    frhd = area;
    frhd->ptr = NULL;
    frhd->size = size / sizeof(HEADER);
    memleft = frhd->size;   /* initial size in four-byte units */
}
#endif


/**
 *  @brief  Data structure for allocation statistics
 */

struct MemStats {
    int freebytes;
    int usedbytes;
    int freeblocks;
    int usedblocks;
    int memleft;
#if 0
    // Not yet
    int largestused;
    int smallestused;
    int largestfree;
    int smallestfree;
#endif
};


/**
 *  @brief  MemStats
 *
 *  @note   Delivers allocation information
 */
void MemStats(struct MemStats *stats) {
HEADER *p;

    stats->memleft     = memleft;
    stats->freeblocks  = 0;
    stats->freebytes   = 0;
    stats->usedblocks  = 0;
    stats->usedbytes   = 0;

    for(p=frhd; p; p=p->ptr) {
        stats->freeblocks++;
        stats->freebytes += p->size*sizeof(HEADER);
    }

    for(p=heapstart; (p < heapend)&&(p->size>0); p=p+p->size) {
        stats->usedblocks++;
        stats->usedbytes += p->size*sizeof(HEADER);
    }
    // Counted in the last loop
    stats->usedblocks -= stats->freeblocks;
    stats->usedbytes  -= stats->freebytes;
}


///////////////////////////////////////////////////////////////////////////////////////////
#define TEST

#ifdef TEST
#include <stdio.h>

/**
 *  @brief  Memory List
 *
 *  @note   List all memory blocks, including size and status
 *
 */
void MemList() {
int i;
HEADER *p;

    for(i=0,p=heapstart;(p<heapend)&&(p->size>0);i++,p=p+p->size) {
        printf("B%02d : %d @%p (next=%p)\n",i,
                    (int) (p->size*sizeof(HEADER)),p,p->ptr);
    }
    putchar('\n');
}

#define PRINTSTATS(MSG,STATS)  \
            do { \
                puts(MSG); \
                MemStats(&STATS); \
                printf("Free blocks      = %d\n",STATS.freeblocks);\
                printf("Free bytes       = %d\n",STATS.freebytes);\
                printf("Used blocks      = %d\n",STATS.usedblocks);\
                printf("Used bytes       = %d\n",STATS.usedbytes);\
                printf("Memory left      = %d\n",(int) (STATS.memleft*sizeof(HEADER)));\
            } while(0)


#define BUFFERSIZE 160
static int buffer[BUFFERSIZE/sizeof(int)];


int main(void) {
char *p1,*p2,*p3;
struct MemStats stats;

    printf("Size of element HEADER = %lu\n",sizeof(HEADER));

    MemInit(buffer,BUFFERSIZE);
    PRINTSTATS("Inicializado",stats);
    MemList();

    p1 = MemAlloc(10);
    PRINTSTATS("Alocacao 1",stats);
    MemList();

    p2 = MemAlloc(10);
    PRINTSTATS("Alocacao 2",stats);
    MemList();

    p3 = MemAlloc(10);
    PRINTSTATS("Alocacao 3",stats);
    MemList();

    MemFree(p2);
    PRINTSTATS("Liberacao 2",stats);
    MemList();

    MemFree(p3);
    PRINTSTATS("Liberacao 3",stats);
    MemList();

    MemFree(p1);
    PRINTSTATS("Liberacao 3",stats);
    MemList();

}
#endif

