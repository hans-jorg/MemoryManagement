
Memory Allocation in C
======================

Introduction
------------


Based on Memory Allocation in C in Embedded Systems Programming, Aug. 89.

The following modification were done:

* Updated to C89/C99
* Added doxygen documentation
* Names changed:
  * free        ->      MemFree
  * malloc      ->      MemAlloc
  * i_alloc     ->      MemInit
* All static variables are now initialized
* Added test routines
* Add routines for statistics 

References
----------

[Lee Aldridge. Memory Allocation in C. Embedded Systems Programming. Aug. 2008](https://www.embedded.com/memory-allocation-in-c/)

[Source code](https://m.eet.com/media/1045620/0808esdAldridge)


Annex
-----

Listing 1  CALLOC.C.

    /*
     * Produced by Programming ARTS
     * 8/18/88
     * Programmers: Les Aldridge, Travis I. Seay
     */

    #define NULL (void *)0

    typedef struct hdr {
        struct hdr	*ptr;
        unsigned int	size;
    } HEADER;

    /*	Defined in the linker file. _heapstart is the first byte allocated to the heap; _heapend
    is the last. */

    extern HEADER _heapstart, _heapend;

    extern void	warm_boot(char *str);

    static HEADER 	*frhd;
    static short 	memleft;	/* memory left */

    void free(char *ap)
    {

    /*	Return memory to free list. Where possible, make contiguous blocks of free memory. (Assumes that 0 is not a valid address for allocation. Also, i_alloc() must be called prior to using either free() or malloc(); otherwise, the free list will be null.) */

        HEADER *nxt, *prev, *f;
        f = (HEADER *)ap - 1;	/* Point to header of block being returned. */
        memleft += f->size;

    /*	frhd is never null unless i_alloc() wasn't called to initialize package. */

    if (frhd > f)
    {

    /* Free-space head is higher up in memory than returnee. */

    nxt = frhd; 	/* old head */
    frhd = f; 	/* new head */
    prev = f + f->size; 	/* right after new head */

    if (prev==nxt) 	/* Old and new are contiguous. */
    f->size += nxt->size;
    f->ptr = nxt->ptr; 	/* Form one block. */
    }
    else f->ptr = nxt;
    return;
    }

    /*	Otherwise, current free-space head is lower in memory. Walk down free-space list looking for the block being returned. If the next pointer points past the block, make a new entry and link it. If next pointer plus its size points to the block, form one contiguous block. */

    nxt = frhd;
    for (nxt=frhd; nxt && nxt < f; prev=nxt,nxt=nxt->ptr)
    {
            if (nxt+nxt->size == f)
            {
                nxt->size += f->size; 	/* They're contiguous. */
                f = nxt + nxt->size; 	/* Form one block. */
                if (f==nxt->ptr)
                {

    /* The new, larger block is contiguous to the next free block, so form a larger block.There's no need to continue this checking since if the block following this free one were free, the two would already have been combined. */

    nxt->size += f->size;
    nxt->ptr = f->ptr;
    }
    return;
        }
    }

    /*	The address of the block being returned is greater than one in the free queue (nxt) or the end of the queue was reached. If at end, just link to the end of the queue. Therefore, nxt is null or points to a block higher up in memory than the one being returned. */

    prev->ptr = f; 	/* link to queue */
    prev = f + f->size; 	/* right after space to free */
    if (prev == nxt) 	/* 'f' and 'nxt' are contiguous. */
    {
        f->size += nxt->size;
        f->ptr = nxt->ptr; 	/* Form a larger, contiguous block. */
    }
    else f->ptr = nxt;
    return;
    }

    char * malloc(int nbytes) 	/* bytes to allocate */
    {
    HEADER *nxt, *prev;
    int 		nunits;
    nunits = (nbytes+sizeof(HEADER)-1) / sizeof(HEADER) + 1;

    /* Change that divide to a shift (for speed) only if the compiler doesn't do it for you, you don't require portability, and you know that sizeof(HEADER) is a power of two. Allocate the space requested plus space for the header of the block. Search the free-space queue for a block that's large enough. If block is larger than needed, break into two pieces and allocate the portion higher up in memory. Otherwise, just allocate the entire block. */

    for (prev=NULL,nxt=frhd; nxt; nxt = nxt->ptr)
    {
        if (nxt->size >= nunits) 	/* big enough */
        {
            if (nxt->size > nunits)
            {
                nxt->size -= nunits; 	/* Allocate tell end. */
                nxt += nxt->size;
                nxt->size = nunits; 	/* nxt now == pointer to be alloc'd. */
            }
            else
            {
                if (prev==NULL) frhd = nxt->ptr;
                else prev->ptr = nxt->ptr;
            }
            memleft -= nunits;

            /* Return a pointer past the header to the actual space requested. */

            return((char *)(nxt+1));
        }
    }

    /* This function that explains what catastrophe befell us before resetting the system. */

    warm_boot("Allocation Failed!");
    return(NULL);
    }

    void i_alloc(void)
    {
    frhd = &_heapstart; 	/* Initialize the allocator. */
    frhd->ptr = NULL;
    frhd->size = ((char *)&_heapend -- (char *)&_heapstart) / sizeof(HEADER);
    memleft = frhd->size; 	/* initial size in four-byte units */
    }
