#ifndef MEMMANAGER_H
#define MEMMANAGER_H
/**
 *  @file   memmanager.h
 *
 *  @brief  header file for memmanager
 */

#include <stdint.h>

/**
 *  @brief  Data structure for allocation statistics
 */

typedef struct memstats {
    uint32_t freebytes;                 ///< Size (in bytes) of total free area
    uint32_t usedbytes;                 ///< Size (in bytes) of total used area
    uint32_t freeblocks;                ///< Number of free blocks
    uint32_t usedblocks;                ///< Number of used blocks
    uint32_t memleft;                   ///< Should be the same of freebytes
    uint32_t largestused;               ///< Largest used block
    uint32_t smallestused;              ///< Smalles used block
    uint32_t largestfree;               ///< Largest free block
    uint32_t smallestfree;              ///< Smalles free block
} MEMSTATS;


/**
 *  @brief  Function prototypes
 */

void MemAddRegion( uint32_t region, void *area, uint32_t size );
void MemInit( void *area, uint32_t size) ;
void MemFree( void *p );
void *MemAlloc( uint32_t nb, uint32_t index );
void MemStats( MEMSTATS *stats, uint32_t region );

#endif  // MEMMANAGER_H
