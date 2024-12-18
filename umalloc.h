#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define ALIGNMENT 16 /* The alignment of all payloads returned by umalloc */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define HEADER_SIZE 32
#define MAGIC_NUM (void *) 0xDEADBEEF
#define MAGIC_NUM_COALESCE (void *) 0xC0A1E5CE

/*
 * memory_block_t - Represents a block of memory managed by the heap. The 
 * struct can be left as is, or modified for your design.
 * In the current design bit0 is the allocated bit
 * bits 1-3 are unused.
 * and the remaining 60 bit represent the size.
 */
typedef struct memory_block_struct {
    size_t block_size_alloc; //will represent the size of PAYLOAD!
    struct memory_block_struct *next;
    struct memory_block_struct *prev;
    uint64_t : 64; //8-byte padding to make the header 16-byte aligned.
} memory_block_t;

// Helper Functions, this may be editted if you change the signature in umalloc.c
bool is_allocated(memory_block_t *block);
void allocate(memory_block_t *block);
void deallocate(memory_block_t *block);
size_t get_size(memory_block_t *block);
memory_block_t *get_next(memory_block_t *block);
void put_block(memory_block_t *block, size_t size, memory_block_t *prev, bool alloc);
void *get_payload(memory_block_t *block);
memory_block_t *get_block(void *payload);
void add_to_alloc_list(memory_block_t *block);
void remove_from_alloc_list(memory_block_t *block);

memory_block_t *find(size_t size);
memory_block_t *extend(size_t size);
memory_block_t *split(memory_block_t *block, size_t size);
memory_block_t *coalesce(memory_block_t *block);


// Portion that may not be edited
int uinit();
void *umalloc(size_t size);
void ufree(void *ptr);