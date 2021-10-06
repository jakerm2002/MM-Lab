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

// A sample pointer to the start of the free list.
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
 * field.
 */
void put_block(memory_block_t *block, size_t size, bool alloc) {
    assert(block != NULL);
    assert(size % ALIGNMENT == 0);
    assert(alloc >> 1 == 0);
    block->block_size_alloc = size | alloc;
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

void add_to_alloc_list(memory_block_t *block) {
    assert(block != NULL);
    assert(is_allocated(block));
    
    //we must change the NEXT pointer of the previous allocated block to point to this block

    if(alloc_head) {
        //we must loop through all allocated blocks to find the 
        //end of the allocated list, which will point to the PREVIOUS ALLOCATED BLOCK

        memory_block_t *cur = alloc_head;
        while(cur->next) {
            cur = cur->next;
        }
        //set the next field of this last element to point to our newly allocated block
        cur->next = block;
        block->next = NULL;
    } else {
        alloc_head = block;
        alloc_head->next = NULL;
    }

    // block->next = NULL;
}

void remove_from_alloc_list(memory_block_t *block) {
    assert(block != NULL);
    // assert(!is_allocated(block));
    assert(alloc_head); //make sure the alloc list is not empty

    //if we are removing the first element, we can simply set
    //alloc_head to alloc_head->next
    if (alloc_head == block) {
        alloc_head = alloc_head->next;
        block->next = NULL;
        return;
    }

    memory_block_t *prev = NULL;
    memory_block_t *cur = alloc_head;
    while(cur != block) {
        prev = cur;
        cur = cur->next;
    }
    prev->next = cur->next;
    cur->next = NULL;

    // while (cur) {
    //     if (cur == block) {
    //         //we must remove this block from the list
    //         prev->next = cur->next;
    //     }
    // }
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
    //uses first-fit algorithm

    //start at the beginning of the free list
    memory_block_t *current_block = free_head;

    //while we still have blocks remaining in the free list
    while(current_block) {
        //check if the malloc call will fit
        int requested_size = ALIGN(size);
        if (get_size(current_block) >= requested_size) {
            //this malloc call will fit, return this block
            return current_block;
        } else {
            //keep going, traverse to the next free block
            *current_block = *current_block->next;
        }
    }

    //if a block is not found, return null
    return NULL;
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t *extend(size_t size) {

    //creates a new block that will fit memory of [size]

    int EXTEND_SIZE = PAGESIZE * 3;
    memory_block_t *new_block;

    if(size > EXTEND_SIZE) {
        new_block = (memory_block_t *) csbrk(ALIGN(size));
        new_block->block_size_alloc = ALIGN(size) - HEADER_SIZE;
    } else {
        new_block = (memory_block_t *) csbrk(PAGESIZE * 3);
        new_block->block_size_alloc = PAGESIZE * 3;
    }
    
    new_block->next = NULL;

    return new_block;
}

/*
 * split - splits a given block in parts, one allocated, one free.
 */
memory_block_t *split(memory_block_t *block, size_t size) {
    return NULL;
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
memory_block_t *coalesce(memory_block_t *block) {
    return NULL;
}



/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {

    // printf("initializing...\n");
    
    
    //call csbrk() with size PAGESIZE * 2 and add it to the free list!
    int INITIAL_SIZE = PAGESIZE * 2;
    free_head = (memory_block_t *) csbrk(INITIAL_SIZE);

    //store (amount of free memory at the beginning of the list) - (header size)
    free_head->block_size_alloc = INITIAL_SIZE - HEADER_SIZE;

    //define that this is the only block in the free list
    free_head->next = NULL; //is this already set to null?

    return 0;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 */
void *umalloc(size_t size) {

    // printf("allocating a block of size\n");
    // printf("%d\n", (int) size);

    memory_block_t *found_block = find(size);

    if(found_block) {
        // printf("found a block, allocating...");
        //allocate the memory
        allocate(found_block);

        //REMOVE THE BLOCK FROM THE FREE LIST
        //assumes first-fit algohirthm
        //if the block is at the beginning of the free list,
        //then we need to set free_head to the next element in the list
        //keep in mind that the next element could be null!
        free_head = found_block->next;

        if(alloc_list) {
            // printf("adding to alloc list\n");
            add_to_alloc_list(found_block);
        } else {
            found_block->next = MAGIC_NUM;
        }

        return get_payload(found_block);
    } else {
        //no memory avaliable, we need to extend
        // printf("no memory avaliable, extending...");
        memory_block_t *new_block = extend(size);
        allocate(new_block);

        if(alloc_list) {
            // printf("adding our brand new memory to alloc list\n");
            add_to_alloc_list(new_block);
        } else {
            new_block->next = MAGIC_NUM;
        }
        
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

    memory_block_t *block = get_block(ptr);
    //if the user tries to free an unallocated block, do nothing
    //we know that the block is unallocated because it will not have a magic number (if not using alloc list)
    if( is_allocated(block) && (alloc_list ? true : &(block->next) == MAGIC_NUM) ) {

        //if we are using the allocated list, we need to remove our block from there
        if (alloc_list) {
            remove_from_alloc_list(block);
        }
        
        //we need to convert this block into a free block
        deallocate(block);

        //if the free list is empty, we can simply set free_head to the block
        if (!free_head) {
            free_head = block;
            block->next = NULL;
            return;
        }


        //figure out where to insert this block into the free list
        memory_block_t *prev_free_block = NULL;
        memory_block_t *current_free_block = free_head;

        while (current_free_block) {
            if (&block < &current_free_block) {
                //this is the place where our block should go

                //check if this block will be at the beginning of the free list
                if (!prev_free_block) {
                    block->next = free_head;
                    free_head = block;
                    return;
                } else {
                    //splice!
                    prev_free_block->next = block;
                    block->next = current_free_block;
                    return;
                }
                
            } else {
                //keep going through the free list
                prev_free_block = current_free_block;
                current_free_block = current_free_block->next;
            }
        }

        //we have reached the end of the list
        //append the free block to the end of the list
        prev_free_block->next = block;
        block->next = NULL;
        return;

    }
    
    //if we reach here, the block was either not set as allocated or did not
    //contain the magic number
}