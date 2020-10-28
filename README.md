
Memory Allocation in C
======================

Introduction
------------


Based on "Memory Allocation in C" in the August, 1989 issue of "Embedded Systems Programming".

The following modification were done:

* Updated to C89/C99
* Added doxygen documentation
* Names changed:
  * free        ->      MemFree
  * malloc      ->      MemAlloc
  * i_alloc     ->      MemInit
* All static variables are now initialized
* Added test routines
* Added routines for statistics 
* Changed all integer types to int32_t/uint32_t (stdint.h)
* Added multiple regions (pools)

References
----------

[Lee Aldridge. Memory Allocation in C. Embedded Systems Programming. Aug. 2008](https://www.embedded.com/memory-allocation-in-c/)

[Source code](https://m.eet.com/media/1045620/0808esdAldridge)


