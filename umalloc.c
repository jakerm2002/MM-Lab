/*

Welcome to my malloc lab! This malloc implementation uses a best-fit algorithm
when searching for free blocks. The code will choose the block that minimizes
the size of the leftover payload. The free list is implemented using a doubly 
linked list, with ONE DUMMY NODE (free_head) whos next field points to the first 
block in the free list. Blocks are inserted into the free list in order of
the memory address of their headers. Blocks are removed from the free list by 
removing references to that block from its previous and next neighbors in the 
free list. free_head->next will point to null if the free list is empty.
The blocks with smaller memory addresses will be
closer to the beginning of the list. My malloc implementation implements
both splitting and coalescing. Splitting is done once the best-fit block is
found. If there is no best fit block, a new block will be created. The new block
may or may not be split (see split method). Coalescing is done immmediately upon
freeing a block. The code attempts to coalesce a block with its previous neighbor
in the free list, and it will then do the same with its next neighbor.

*/

#include "umalloc.h"
#include "csbrk.h"
#include "ansicolors.h"
#include <stdio.h>
#include <assert.h>

const char author[] = ANSI_BOLD ANSI_COLOR_RED "Jake Medina jrm7784" ANSI_RESET;

/*
 * The following helpers can be used to interact with the memory_block_t
 * struct, they can be adjusted as necessary.
 */

// A pointer to the HEADER NODE of the free list.
memory_block_t *free_head;

// A pointer to the start of the allocated list.
memory_block_t *alloc_head;

// Set to true if we wish to utilize an allocated list
// to track all allocated blocks.
// Used for completing methods in check_heap().
bool alloc_list = false;

/*
 * is_allocated - returns true if a block is marked as allocated.
 */
bool is_allocated(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & 0x1;
}

/*
 * allocate - marks a block as allocated.
 */
void allocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc |= 0x1;
}


/*
 * deallocate - marks a block as unallocated.
 */
void deallocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc &= ~0x1;
}

/*
 * get_size - gets the size of the block.
 */
size_t get_size(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & ~(ALIGNMENT-1);
}

/*
 * get_next - gets the next block.
 */
memory_block_t *get_next(memory_block_t *block) {
    assert(block != NULL);
    return block->next;
}

/*
 * put_block - puts a block struct into memory at the specified address.
 * Initializes the size and allocated fields, along with NUlling out the next 
 * field. Sets the block's prev field.
 */
void put_block(memory_block_t *block, size_t size, memory_block_t *prev, bool alloc) {
    assert(block != NULL);
    assert(size % ALIGNMENT == 0);
    assert(alloc >> 1 == 0);
    block->block_size_alloc = size | alloc;
    block->prev = prev;
    block->next = NULL;
}

/*
 * get_payload - gets the payload of the block.
 */
void *get_payload(memory_block_t *block) {
    assert(block != NULL);
    return (void*)(block + 1);
}

/*
 * get_block - given a payload, returns the block.
 */
memory_block_t *get_block(void *payload) {
    assert(payload != NULL);
    return ((memory_block_t *)payload) - 1;
}

/*
 * The following are helper functions that can be implemented to assist in your
 * design, but they are not required. 
 */

/*
 * find - finds a free block that can satisfy the umalloc request.
 */
memory_block_t *find(size_t size) {

    //we must traverse through our free list to see if there is a fit
    //uses BEST-FIT algorithm
    //use a pointer to point to the best (smallest) block we have found so far
    memory_block_t *best = NULL;
    memory_block_t *cur = free_head->next;
    
    int requested_size = ALIGN(size);
    //loop thru the entire free list
    while(cur) {
        assert(!is_allocated(cur));
        assert(cur != MAGIC_NUM);
        //check if the malloc call will fit AND check if the size 
        //is smaller than the previous best fit
        if (get_size(cur) >= requested_size) {
            //this malloc call will fit, is it a better fit than the previous block?
            if (!best || get_size(cur) < get_size(best)) {
                //this block is a better fit
                best = cur;
            }
        }
        cur = cur->next;
    }

    assert(best != free_head);
    return best;
}

/*
 * extend - extends the heap if more memory is required.
 * attempts to create a new block of default size PAGESIZE * 3.
 * will split the block if the size requested is smaller than PAGESIZE * 3.
 * otherwise, will create a block for an exact fit of the requested size.
 */
memory_block_t *extend(size_t size) {

    int EXTEND_SIZE = PAGESIZE * 3;
    memory_block_t *new_block;

    if(size + HEADER_SIZE > EXTEND_SIZE) {
        new_block = (memory_block_t *) csbrk(ALIGN(size + HEADER_SIZE));
        put_block(new_block, ALIGN(size + HEADER_SIZE) - HEADER_SIZE, MAGIC_NUM, true);
        new_block->next = MAGIC_NUM;
        return new_block;
    } else {
        new_block = (memory_block_t *) csbrk(EXTEND_SIZE);
        new_block->block_size_alloc = EXTEND_SIZE - HEADER_SIZE;

        //split this extended block into two parts: one for allocation, and one to be added to the free list.
        int block_t_size = new_block->block_size_alloc + HEADER_SIZE;
        int a_block_t_size = ALIGN(size) + HEADER_SIZE;
        int f_block_t_size = block_t_size - a_block_t_size;
        
        if (f_block_t_size > HEADER_SIZE) {
            //split this block.
            put_block(new_block, a_block_t_size - HEADER_SIZE, MAGIC_NUM, true);
            new_block->next = MAGIC_NUM;

            //the remaining part goes to the free list
            //get a pointer to end of the free list
            memory_block_t *free_end = free_head;
            while (free_end->next) {
                free_end = free_end->next;
            }

            memory_block_t *f_block = (void *) new_block + a_block_t_size;
            free_end->next = f_block;
            put_block(f_block, f_block_t_size - HEADER_SIZE, free_end, false); //next pointer will be NULL
            return new_block;
        } else {
            //split block is does not have enough left over for a free block.
            //just allocate the whole block.
            put_block(new_block, new_block->block_size_alloc, MAGIC_NUM, true);
            new_block->next = MAGIC_NUM;
            return new_block;
        }
    }
}

/*
 * split - splits a given block in parts, one FOR ALLOCATION, one free.
 * pre: size must be ALIGNMENT-byte aligned.
 * If the block does not have room to be split, return the same block.
 */
memory_block_t *split(memory_block_t *block, size_t size) {
    assert(!is_allocated(block));
    assert(block->prev != MAGIC_NUM);
    assert(block->next != MAGIC_NUM);
    assert(size % ALIGNMENT == 0);

    int block_t_size = block->block_size_alloc + HEADER_SIZE;
    int a_block_t_size = size + HEADER_SIZE;
    int f_block_t_size = block_t_size - a_block_t_size;

    memory_block_t *block_prev = block->prev;
    memory_block_t *block_next = block->next;

    if (f_block_t_size > HEADER_SIZE) {
        //this block goes to allocation
        put_block(block, size, MAGIC_NUM, true);
        block->next = MAGIC_NUM;

        //the remaining part goes to the free list
        memory_block_t *f_block = (void *) block + a_block_t_size;
        put_block(f_block, f_block_t_size - HEADER_SIZE, block_prev, false);
        f_block->next = block_next;

        //set the free list pointers to our free block
        block_prev->next = f_block;
        if (block_next) {
            block_next->prev = f_block;
        }
        return block;
    } else {
        memory_block_t *f_block = block->next;

        //just allocate the whole block bruh
        put_block(block, block->block_size_alloc, MAGIC_NUM, true);
        block->next = MAGIC_NUM;

        //set the free list pointers to skip over this free block
        block_prev->next = f_block;
        if (f_block) {
            f_block->prev = block_prev; 
        }
        return block;
    }
}

/*
 * coalesce_prev - coalesces a free memory block with its previous neighbor.
 */
memory_block_t *coalesce_prev(memory_block_t *block) {
    
    //we want to make sure we don't coalesce with the HEADER node!
    //we also want to make sure the blocks are contiguous in memory.
    if(block->prev != free_head && ((void *) block->prev + HEADER_SIZE + block->prev->block_size_alloc == (void *) block)) {
        int prev_block_t_size = block->prev->block_size_alloc + HEADER_SIZE;
        int this_block_t_size = block->block_size_alloc + HEADER_SIZE;
        int new_block_t_size = prev_block_t_size + this_block_t_size;

        memory_block_t *prev_block = block->prev;
        memory_block_t *next_block = block->next;

        put_block(prev_block, new_block_t_size - HEADER_SIZE, prev_block->prev, false);

        //update the free list pointers
        prev_block->prev->next = prev_block; //this is technically unnecessary
        prev_block->next = next_block;
        if (next_block) {
            next_block->prev = prev_block;
        }
        return prev_block;
    } else {
        //if the block was not coalesced, return the unchanged block
        return block;
    }
}

/*
 * coalesce_prev - coalesces a free memory block with its next neighbor.
 */
memory_block_t *coalesce_next(memory_block_t *block) {
    //we want to make sure that there is a next block!
    //we also want to make sure the blocks are contiguous in memory.

    if(block->next && ((void *) block + HEADER_SIZE + block->block_size_alloc == (void *) block->next)) {
        int this_block_t_size = block->block_size_alloc + HEADER_SIZE;
        int next_block_t_size = block->next->block_size_alloc + HEADER_SIZE;
        int new_block_t_size = this_block_t_size + next_block_t_size;

        memory_block_t *this_block = block;
        memory_block_t *next_next_block = block->next->next;

        put_block(this_block, new_block_t_size - HEADER_SIZE, this_block->prev, false);

        //update the free list pointers
        this_block->prev->next = this_block; //this is technically unnecessary
        this_block->next = next_next_block;
        if (next_next_block) {
            next_next_block->prev = this_block;
        }
        return this_block;
    } else {
        //if the block was not coalesced, return the unchanged block
        return block;
    }
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
memory_block_t *coalesce(memory_block_t *block) {
    memory_block_t *c_block = coalesce_prev(block);
    c_block = coalesce_next(c_block);
    return c_block;
}



/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    
    //call csbrk() with size PAGESIZE * 2 and add it to the free list!
    int INITIAL_SIZE = PAGESIZE * 2;

    //this block of memory is used exclusively for our header node
    free_head = (memory_block_t *) csbrk(32);
    memory_block_t *init_block = (memory_block_t *) csbrk(INITIAL_SIZE);

    //store (amount of free memory at the beginning of the list) - (header size)
    //define that this is the only block in the free list (prev = free_head)
    put_block(init_block, INITIAL_SIZE - HEADER_SIZE, free_head, false);
    free_head->next = init_block;
    return 0;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 */
void *umalloc(size_t size) {
    memory_block_t *found_block = find(size);
    //points to a block of at least ALIGN(size)

    if(found_block) {
        //we will split the block here
        //pass in an aligned size parameter, i.e. ALIGN(size)
        found_block = split(found_block, ALIGN(size));
        return get_payload(found_block);
    } else {
        //no memory avaliable, we need to extend
        memory_block_t *new_block = extend(size);
        return get_payload(new_block);
    }
}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {
    memory_block_t *block = get_block(ptr);
    //make sure we are not trying to free an unallocated block
    assert(is_allocated(block));

    if( is_allocated(block) && (block->next == MAGIC_NUM) ) {
        //we need to convert this block into a free block
        deallocate(block);
        memory_block_t *prev = free_head;
        memory_block_t *cur = free_head->next;

        while(cur) {
            if (block < cur) {
                //this is the correct place to insert our block
                prev->next = block;
                block->prev = prev;
                block->next = cur;
                cur->prev = block;
                coalesce(block);
                return;
            } else {
                //keep going...
                prev = cur;
                cur = cur->next;
            }
        }

        //we have reached the end, this block should be at the end of the list
        prev->next = block;
        block->prev = prev;
        block->next = cur;
        coalesce(block);
        return;
    }
}