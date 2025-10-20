#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "liborkh_gpu_elf_pool.h"
#include "liborkh_utils.h"

liborkh_status_t liborkh_gpu_elf_pool_init(liborkh_gpu_elf_pool_t **pool, size_t initial_capacity)
{
    LIBORKH_CHECK_ARGUMENTS(!pool || initial_capacity == 0);

    *pool = malloc(sizeof(liborkh_gpu_elf_pool_t));
    LIBORKH_CHECK_ALLOC(*pool);

    (*pool)->entries = calloc(initial_capacity, sizeof(liborkh_gpu_elf_pool_t*));
    if (!(*pool)->entries) { free(*pool); }
    LIBORKH_CHECK_ALLOC((*pool)->entries);

    (*pool)->count = 0;
    (*pool)->capacity = initial_capacity;
    return LIBORKH_SUCCESS;
}

liborkh_status_t liborkh_gpu_elf_pool_free(liborkh_gpu_elf_pool_t *pool)
{
    LIBORKH_CHECK_ARGUMENTS(!pool);

    for (size_t i = 0; i < pool->count; i++) {
        liborkh_free_entry(pool->entries[i]);
    }
    free(pool->entries);
    free(pool);
    return LIBORKH_SUCCESS;
}

liborkh_status_t liborkh_new_entry(liborkh_gpu_elf_entry_t **entry)
{
    LIBORKH_CHECK_ARGUMENTS(!entry);

    *entry = malloc(sizeof(liborkh_gpu_elf_entry_t));
    LIBORKH_CHECK_ALLOC(*entry);

    (*entry)->id = 0;
    (*entry)->img = IMG_None;
    (*entry)->ofk = OFK_None;
    (*entry)->target_triple_size = 0;
    (*entry)->target_triple = NULL;
    (*entry)->target_arch_size = 0;
    (*entry)->target_arch = NULL;
    (*entry)->elf_size = 0;
    (*entry)->elf = NULL;
    return LIBORKH_SUCCESS;
}

liborkh_status_t liborkh_free_entry(liborkh_gpu_elf_entry_t *entry)
{
    LIBORKH_CHECK_ARGUMENTS(!entry);

    if (entry->target_triple) free(entry->target_triple);
    if (entry->target_arch)   free(entry->target_arch);
    if (entry->elf)           free(entry->elf);
    free(entry);
    return LIBORKH_SUCCESS;
}


liborkh_status_t liborkh_gpu_elf_pool_pop(liborkh_gpu_elf_pool_t *pool, liborkh_gpu_elf_entry_t **entry)
{
    LIBORKH_CHECK_ARGUMENTS(!pool);
    if (pool->count == 0) return LIBORKH_ERROR_POOL_EMPTY;

    liborkh_gpu_elf_entry_t* popped_entry = pool->entries[pool->count--];

    if (entry) {
        *entry = popped_entry;
    } else {
        liborkh_free_entry(popped_entry);
    }
    return LIBORKH_SUCCESS;
}


liborkh_status_t liborkh_gpu_elf_pool_push(liborkh_gpu_elf_pool_t *pool, liborkh_gpu_elf_entry_t *entry)
{
    LIBORKH_CHECK_ARGUMENTS(!pool);

    if (!entry) return LIBORKH_SUCCESS;

    if (pool->count >= pool->capacity) {
        size_t new_capacity = pool->capacity * 2;
        liborkh_gpu_elf_entry_t **tmp = (liborkh_gpu_elf_entry_t**) realloc(pool->entries, new_capacity * sizeof(liborkh_gpu_elf_entry_t*));
        LIBORKH_CHECK_ALLOC(tmp);
        
        pool->entries = tmp;
        pool->capacity = new_capacity;
    }
    pool->entries[pool->count++] = entry;
    return LIBORKH_SUCCESS;
}


liborkh_status_t liborkh_gpu_elf_pool_iterate(liborkh_gpu_elf_pool_t *pool, liborkh_gpu_elf_pool_iterate_cb_t func, void *user_data)
{
    LIBORKH_CHECK_ARGUMENTS(!pool || !func);

    for (size_t i = 0; i < pool->count; i++) {
        LIBORKH_CHECK_CALL(func(pool->entries[i], user_data), "Error processing entry at index %zu\n", i);
    }
    return LIBORKH_SUCCESS;
}
