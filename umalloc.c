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
    
    //uses best-fit algorithm
    
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
            //keep going until the end
        }
        cur = cur->next;
    }

    return best;
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t *extend(size_t size) {

    //creates a new block that will fit memory of [size]

    //NOTE: the memory from this method will NOT be added to the 
    //free list because it's purpose is to be allocated immediately

    int EXTEND_SIZE = PAGESIZE * 3;
    memory_block_t *new_block;

    if(size + HEADER_SIZE > EXTEND_SIZE) {
        new_block = (memory_block_t *) csbrk(ALIGN(size + HEADER_SIZE));
        // printf("extend, creating block of size %ld\n", ALIGN(size + HEADER_SIZE));

        new_block->block_size_alloc = ALIGN(size + HEADER_SIZE) - HEADER_SIZE;
        // printf("block_size_alloc is %ld\n", get_size(new_block));
    } else {
        new_block = (memory_block_t *) csbrk(EXTEND_SIZE);
        new_block->block_size_alloc = EXTEND_SIZE - HEADER_SIZE;
    }

    new_block->prev = NULL;
    new_block->next = NULL;

    return new_block;
}

/*
 * split - splits a given block in parts, one FOR ALLOCATION, one free.
 * pre: size must be ALIGNMENT-byte aligned.
 * If the block does not have room to be split, return the same block.
 */
memory_block_t *split(memory_block_t *block, size_t size) {

    int requested_size = size + HEADER_SIZE; //we MUST have at least this total block size
    int f_block_total_size = block->block_size_alloc + HEADER_SIZE - requested_size;

    printf("attempting to split this block of T_SIZE %ld\n", block->block_size_alloc + HEADER_SIZE);
    printf("into size %d", requested_size);
    printf("and %d\n", f_block_total_size);

    assert(!is_allocated(block));
    assert(size % ALIGNMENT == 0);
    
    // if (f_block_total_size > 32) {
    //     printf("splitting block...\n");
    //     memory_block_t *f_block = (void *) block + requested_size; //portion of the block to be left unallocated
    //     put_block(f_block, block->block_size_alloc - requested_size, block, false);

    //     block->block_size_alloc = requested_size - HEADER_SIZE;
    //     block->next = f_block;

    //     printf("successfully split block into P_SIZE %ld\n", block->block_size_alloc);
    //     printf("and %ld\n", f_block->block_size_alloc);
    // }

    if (f_block_total_size > HEADER_SIZE) {
        printf("splitting block...\n");
        memory_block_t *f_block = (void *) block + requested_size; //portion of the block to be left unallocated
        put_block(f_block, f_block_total_size - HEADER_SIZE, block, false);

        f_block->next = block->next;

        block->block_size_alloc = requested_size - HEADER_SIZE;
        block->next = f_block;

        printf("successfully split block into P_SIZE %ld\n", block->block_size_alloc);
        printf("and %ld\n", f_block->block_size_alloc);
    }


    return block;
}

/*
 * coalesce_prev - coalesces a free memory block with its previous neighbor.
 */
memory_block_t *coalesce_prev(memory_block_t *block) {
    //we want to make sure we don't coalesce with the HEADER node!
    // memory_block_t *c_block;
    if(block->prev != free_head && (block->prev + HEADER_SIZE + block->prev->block_size_alloc == block)) {
        assert(block->prev);
        printf("coalescing prev...\n");
        printf("BLOCK 1 T_SIZE = %ld\n", block->prev->block_size_alloc + HEADER_SIZE);
        printf("BLOCK 2 T_SIZE = %ld\n", block->block_size_alloc + HEADER_SIZE);
        printf("coalescing blocks with START/END address\n");
        printf("&BLOCK 1 START = %p\n", (void *) block->prev);
        printf("&BLOCK 1 END = %p\n", (void *) block->prev + HEADER_SIZE + block->prev->block_size_alloc);
        // c_block = block->prev; //points to the beginning of the block before this one

        printf("&BLOCK 2 START = %p\n", (void *) block);
        printf("&BLOCK 2 END = %p\n", (void *) block + HEADER_SIZE + block->block_size_alloc);

        block->prev->next = block->next;
        if(block->next) {
            block->next->prev = block->prev;
        }
        
        //change the size of the prev block to include this one
        block->prev->block_size_alloc = get_size(block->prev) + get_size(block) + HEADER_SIZE;

        //remove the pointers from the block
        //and change to a magic num for debugging
        // block->prev = MAGIC_NUM_COALESCE;
        // block->next = MAGIC_NUM_COALESCE;

        printf("NEW BLOCK T_SIZE = %ld\n", block->prev->block_size_alloc + HEADER_SIZE);
        printf("&NEW BLOCK START = %p\n", (void *) block->prev);
        printf("&NEW BLOCK END = %p\n", (void *) block->prev + HEADER_SIZE + block->prev->block_size_alloc);
        return block->prev;
    } else {
        printf("no prev coalesce\n");
    }
    //if the block was not coalesced, return the unchanged block
    return block;
}

/*
 * coalesce_next - coalesces a free memory block with its next neighbor.
 */
memory_block_t *coalesce_next(memory_block_t *block) {
    assert(!is_allocated(block));

    if(block->next != NULL && (block + HEADER_SIZE + block->block_size_alloc == block->next)) {
        assert(!is_allocated(block->next));
        assert(block < block->next);
        printf("coalescing next...\n");
        printf("BLOCK 1 T_SIZE = %ld\n", block->block_size_alloc + HEADER_SIZE);
        printf("BLOCK 2 T_SIZE = %ld\n", block->next->block_size_alloc + HEADER_SIZE);
        printf("coalescing blocks with START/END address\n");
        printf("&BLOCK 1 START = %p\n", (void *) block);
        printf("&BLOCK 1 END = %p\n", (void *) block + HEADER_SIZE + block->block_size_alloc);
        
        block->block_size_alloc = get_size(block) + get_size(block->next) + HEADER_SIZE;

        // memory_block_t *c_block = (void *) block->next;
        printf("&BLOCK 2 START = %p\n", (void *) block->next);
        printf("&BLOCK 2 END = %p\n", (void *) block->next + HEADER_SIZE + block->next->block_size_alloc);
        // block->next = c_block->next;
        block->next = block->next->next;
        // c_block->prev = MAGIC_NUM_COALESCE;
        // c_block->next = MAGIC_NUM_COALESCE;

        printf("NEW BLOCK T_SIZE = %ld\n", block->block_size_alloc + HEADER_SIZE);
        printf("&NEW BLOCK START = %p\n", (void *) block);
        printf("&NEW BLOCK END = %p\n", (void *) block + HEADER_SIZE + block->block_size_alloc);
        return block;
    }
    return block;
    //if the block was not coalesced, return the unchanged block
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
memory_block_t *coalesce(memory_block_t *block) {
    assert(!is_allocated(block));

    //pointer to the coalesced block if operation was successful
    memory_block_t *prev_coalesce = coalesce_prev(block);
    memory_block_t *result = coalesce_next(prev_coalesce);

    return result;
}



/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {

    // printf("initializing...\n");
    
    
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

    // printf("allocating a block of size\n");
    // printf("%d\n", (int) size);

    memory_block_t *found_block = find(size);
    //points to a block of at least ALIGN(size)

    if(found_block) {

        //we will split the block here
        //pass in an aligned size parameter, i.e. ALIGN(size)

        //do not split if ALIGN(size) = found_block->block_size_alloc + HEADER_SIZE
        //this will create a pointer to nothing
        // printf("splitting...\n");
        found_block = split(found_block, ALIGN(size));

        printf("found and allocating a block of T_SIZE :%ld\n",found_block->block_size_alloc + HEADER_SIZE);
        //allocate the memory
        allocate(found_block);

        //REMOVE THE BLOCK FROM THE FREE LIST
        //asssumes best-fit algorithm
        found_block->prev->next = found_block->next;

        //special case: block is at the end of the free list
        if(found_block->next) {
            found_block->next->prev = found_block->prev;
        }

        found_block->next = MAGIC_NUM;
        found_block->prev = MAGIC_NUM;

        return get_payload(found_block);
    } else {
        //no memory avaliable, we need to extend
        memory_block_t *new_block = extend(size);
        printf("no memory avaliable, creating new block of T_SIZE: %ld\n", new_block->block_size_alloc + HEADER_SIZE);
        //NOTE: the memory from extend will never be added to the free list
        //because it's purpose is to be allocated immediately
        
        allocate(new_block);

        new_block->prev = MAGIC_NUM;
        new_block->next = MAGIC_NUM;

        
        //DO LATER - split the new block?
        //we would have to add the part that we don't use to the free list

        return get_payload(new_block);
    }

}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {

    // printf("free");
    memory_block_t *block = get_block(ptr);
    //if the user tries to free an unallocated block, do nothing
    //we know that the block is unallocated because it will not have a magic number (if not using alloc list)
    assert(is_allocated(block));

    // if (is_allocated(block)) {
    //     printf("allocated");
    // }
    // if ((block->next) == MAGIC_NUM) {
    //     printf("true.");
    // }
    // printf("(&(block->next) == MAGIC_NUM));
    if( is_allocated(block) && (block->next == MAGIC_NUM) ) {
        printf("Hello.");
        
        //we need to convert this block into a free block
        deallocate(block);

        //if the free list is empty, we can simply set free_head->next to the block
        if (!free_head->next) {
            printf("free list empty\n");
            free_head->next = block;
            block->prev = free_head;
            block->next = NULL;
            // coalesce(block);
            assert(block->prev != MAGIC_NUM);
            assert(block->next != MAGIC_NUM);
            return;
        }


        //figure out where to insert this block into the free list
        memory_block_t *prev_free_block = NULL;
        memory_block_t *current_free_block = free_head->next;

        while (current_free_block) {
            // if (&block < &current_free_block) {
            if (block < current_free_block) {
                //this is the place where our block should go

                //check if this block will be at the beginning of the free list
                if (!prev_free_block) {
                    printf("hello");
                    block->prev = free_head;
                    block->next = free_head->next;
                    free_head->next = block;
                    // coalesce(block);
                    assert(block->prev != MAGIC_NUM);
                    assert(block->next != MAGIC_NUM);
                    return;
                } else {
                    //splice!
                    printf("splicing...");
                    prev_free_block->next = block;
                    block->prev = prev_free_block;
                    block->next = current_free_block;
                    // coalesce(block);
                    assert(block->prev != MAGIC_NUM);
                    assert(block->next != MAGIC_NUM);
                    return;
                }
                
            } else {
                printf("continuing...\n");
                //keep going through the free list
                prev_free_block = current_free_block;
                current_free_block = current_free_block->next;
            }
        }

        //we have reached the end of the list
        //append the free block to the end of the list
        prev_free_block->next = block;
        block->prev = prev_free_block;
        block->next = NULL;
        // coalesce(block);`
        assert(block->prev != MAGIC_NUM);
        assert(block->next != MAGIC_NUM);
        return;

    }
    
    //if we reach here, the block was either not set as allocated or did not
    //contain the magic number
}