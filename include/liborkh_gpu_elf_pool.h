#ifndef LIBORKH_GPU_ELF_POOL_H
#define LIBORKH_GPU_ELF_POOL_H

#include <stdint.h>
#include "liborkh_utils.h"

typedef struct {
    size_t count;
    size_t capacity;
    liborkh_gpu_elf_entry_t** entries;
} liborkh_gpu_elf_pool_t;

typedef liborkh_status_t (*liborkh_gpu_elf_pool_iterate_cb_t)(liborkh_gpu_elf_entry_t *entry, void *user_data);

liborkh_status_t liborkh_gpu_elf_pool_init(liborkh_gpu_elf_pool_t **pool, size_t initial_capacity);
liborkh_status_t liborkh_gpu_elf_pool_free(liborkh_gpu_elf_pool_t *pool);
liborkh_status_t liborkh_gpu_elf_pool_pop(liborkh_gpu_elf_pool_t *pool, liborkh_gpu_elf_entry_t **entry);
liborkh_status_t liborkh_gpu_elf_pool_push(liborkh_gpu_elf_pool_t *pool, liborkh_gpu_elf_entry_t *entry);
liborkh_status_t liborkh_gpu_elf_pool_iterate(liborkh_gpu_elf_pool_t *pool, liborkh_gpu_elf_pool_iterate_cb_t func, void *user_data);
liborkh_status_t liborkh_new_entry(liborkh_gpu_elf_entry_t **entry);
liborkh_status_t liborkh_free_entry(liborkh_gpu_elf_entry_t *entry);

#endif // LIBORKH_GPU_ELF_POOL_H