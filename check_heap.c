
#include "umalloc.h"
#include <stdio.h>

//Place any variables needed here from umalloc.c as an extern.
extern memory_block_t *free_head;
// extern memory_block_t *alloc_head;

/*
 * check_alignment - used to ensure that every block in a list of
 * memory_block_t structs is aligned to the ALIGNMENT-byte
 * requirement. Will return -1 if any block is NOT aligned,
 * returns 0 otherwise.
*/
int check_alignment(memory_block_t *block) {
    unsigned long mem_address = 0;
    while(block) {
        mem_address = (unsigned long) block;
        if (mem_address % ALIGNMENT != 0) {
            return -1;
        }
        block = block->next;
    }
    return 0;
}

/*
 * check_heap -  used to check that the heap is still in a consistent state.
 * Required to be completed for checkpoint 1.
 * Should return 0 if the heap is still consistent, otherwise return a non-zero
 * return code. Asserts are also a useful tool here.
 */
int check_heap() {

    // Example heap check:

    //HEAP CHECK #1
    // Check that all blocks in the free list are marked free.
    // If a block is marked allocated, return 10.
    memory_block_t *cur = free_head;
    while (cur) {
        if (is_allocated(cur)) {
            return 10;
        }
        cur = cur->next;
    }


    //HEAP CHECK #2
    // Check that all free and allocated blocks are in the correct alignment.
    // If a free block is out of alignment, return 20.
    // If an allocated block is out of alignment, return 25.

    //check all free blocks
    memory_block_t *block = free_head;
    if (check_alignment(block)) {
        return 20;
    }

    // //check all allocated blocks
    // block = alloc_head;
    // if (check_alignment(block)) {
    //     return 25;
    // }
    

    //HEAP CHECK #3
    // Check that the free list is being maintained in memory 
    // order.
    // If a free block is out of sorted memory order in the free list,
    // return 30.

    memory_block_t *prev = NULL;
    cur = free_head;
    while(cur) {
        //first two statements make sure we are not comparing a memory address 
        //to NULL in the case that there is only one block in the free list OR
        //we are at the end of the free list
        if(!prev && !cur && (prev >= cur)) {
            return 30;
        }
        //moves the pointers to the next pair of blocks
        prev = cur;
        cur = cur->next;
    }


    //HEAP CHECK #4
    // Check if any allocated blocks overlap with each other.
    // Returns either 40 or 45 depending if the overlap is found
    // near the beginning or end of a block.

    // //need two linked list pointers
    // cur = alloc_head;

    // //checks each allocated block with all other allocated blocks.
    // while (cur) {
    //     memory_block_t *loop = alloc_head;
    //     while (loop) {
    //         //ensure that we are not checking a block with itself
    //         if (cur != loop) {
    //             //check if start_address(cur) is within address range of [loop] (indicates overlap)
    //             if ( (cur >= loop) && ((void *)cur < (void *)loop + get_size(loop)) ) {
    //                 return 40;
    //             }

    //             //check if end_address(cur) is within address range of [loop] (indicates overlap)
    //             if ( ((void *)cur + get_size(cur) > (void *)loop) && ((void *)cur + get_size(cur) <= (void *)loop + get_size(loop)) ) {
    //                 return 45;
    //             }
    //         }
    //         loop = loop->next;
    //     }
    //     cur = cur->next;
    // }


    return 0;
}