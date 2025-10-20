#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "liborkh_utils.h"
#include "liborkh_gpu_elf_pool.h"
#include "liborkh_log.h"
#include "liborkh_elf_utils.h"
#include "liborkh_kernel_metadata.h"


liborkh_status_t liborkh_get_kernels_metadata(Elf* elf, uint8_t** out_metadata, size_t* out_size) {
    LIBORKH_CHECK_ARGUMENTS(!elf || !out_metadata || !out_size);

    elf_note_t note = {0};
    LIBORKH_CHECK_CALL(liborkh_get_note_data(elf, &note), "Cannot get .note data\n");
  
    free(note.name);
    *out_size = note.descsz;
    *out_metadata = note.desc;

    return LIBORKH_SUCCESS;
}


liborkh_status_t liborkh_get_number_kernels(const uint8_t* metadata, size_t metadata_size, size_t* out_num_kernels) {
    LIBORKH_CHECK_ARGUMENTS(!out_num_kernels || !metadata || metadata_size == 0);

    *out_num_kernels = 0;

    for (size_t i = 0; i + LIBORKH_AMDHSAMETADATA_KEY_SIZE < metadata_size; i++) {
        if (memcmp(metadata + i, LIBORKH_AMDHSAMETADATA_KEY, LIBORKH_AMDHSAMETADATA_KEY_SIZE) == 0) {

            // Array starts immediately after the key
            const uint8_t *p = metadata + i + LIBORKH_AMDHSAMETADATA_KEY_SIZE;
            uint8_t b = p[0];

            // fixarray
            if ((b & 0xf0) == 0x90) {
                *out_num_kernels = b & 0x0f;
                break;
            } 
            // array16
            else if (b == 0xdc) {
                *out_num_kernels = (p[1] << 8) | p[2];
                break;
            } 
            // array32
            else if (b == 0xdd) {
                *out_num_kernels = ((size_t)p[1] << 24) | ((size_t)p[2] << 16) | ((size_t)p[3] << 8) | p[4];
                break;
            } 
            // Not an array
            else {
                liborkh_log_err("amdhsa.kernels key not followed by an array\n");
                return LIBORKH_ERROR_METADATA_NOT_FOUND;
            }
        }
    }

    return LIBORKH_SUCCESS;
}


liborkh_status_t liborkh_get_number_kernels_in_pool(liborkh_gpu_elf_pool_t* pool, size_t* out_num_kernels) {
    LIBORKH_CHECK_ARGUMENTS(!pool || !out_num_kernels);

    *out_num_kernels = 0;

    for (size_t i = 0; i < pool->count; ++i) {
        liborkh_gpu_elf_entry_t *entry = pool->entries[i];

        if (!entry->elf || entry->elf_size == 0) {
            continue;
        }

        Elf * gpu_elf = NULL;
        LIBORKH_CHECK_CALL(liborkh_open_elf_from_memory(entry->elf, entry->elf_size, &gpu_elf), "Cannot open ELF from memory for pool entry %zu\n", i);

        uint8_t *metadata = NULL;
        size_t metadata_size = 0;
        LIBORKH_CHECK_CALL(liborkh_get_kernels_metadata(gpu_elf, &metadata, &metadata_size), "Cannot get kernel metadata from ELF in pool entry %zu\n", i);

        size_t num_kernels = 0;
        LIBORKH_CHECK_CALL(liborkh_get_number_kernels(metadata, metadata_size, &num_kernels), "Cannot get number of kernels from metadata in pool entry %zu\n", i);

        *out_num_kernels += num_kernels;

        liborkh_close_elf(gpu_elf);
        free(metadata);
    }

    return LIBORKH_SUCCESS;
}
