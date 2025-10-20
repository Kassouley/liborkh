#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include "liborkh.h"


liborkh_status_t print_nb_kernels_in_entry(liborkh_gpu_elf_entry_t *entry, void *user_data) {
    uint8_t *metadata = NULL;
    size_t metadata_size = 0;
    Elf * gpu_elf = NULL;
    size_t num_kernels = 0;

    char* gpu_elf_name = liborkh_get_elf_name(entry, user_data);

    if (!entry->elf || entry->elf_size == 0) {
        liborkh_log_warn("Skipping entry %s: empty ELF data\n", gpu_elf_name);
        return LIBORKH_SUCCESS;
    }

    LIBORKH_CHECK_CALL(liborkh_open_elf_from_memory(entry->elf, entry->elf_size, &gpu_elf), "Cannot open ELF from memory for entry %s\n", gpu_elf_name);
    LIBORKH_CHECK_CALL(liborkh_get_kernels_metadata(gpu_elf, &metadata, &metadata_size),    "Cannot get kernel metadata from ELF in entry %s\n", gpu_elf_name);
    LIBORKH_CHECK_CALL(liborkh_get_number_kernels(metadata, metadata_size, &num_kernels),   "Cannot get number of kernels from metadata in entry %s\n", gpu_elf_name);

    liborkh_log_info("Number of kernels in entry %s: %u\n", gpu_elf_name, num_kernels);
    
    liborkh_close_elf(gpu_elf);
    free(metadata);
    free(gpu_elf_name);

    return LIBORKH_SUCCESS;
}


int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <input ELF file>\n", argv[0]);
        return 1;
    }

    char *elf_filename = argv[1];

    Elf *elf = NULL;
    if (liborkh_open_elf(elf_filename, &elf) != 0) {
        return 1;
    }

    liborkh_offload_buffer fatbin_buf = {0};
    if (liborkh_extract_gpu_fatbin(elf, &fatbin_buf) != 0) {
        liborkh_close_elf(elf);
        return 1;
    }

    char *elf_basename = basename(elf_filename);

    liborkh_close_elf(elf);

    liborkh_gpu_elf_pool_t *pool = NULL;
    if (liborkh_gpu_elf_pool_init(&pool, 4) != 0) {
        free(fatbin_buf.buf);
        return 1;
    }

    liborkh_entry_filter_t filter = {0};
    filter.id_mode = FILTER_ID_MODE_ONE_BY_ID;
    if (liborkh_get_gpu_elfs(&fatbin_buf, pool, &filter) != 0) {
        free(fatbin_buf.buf);
        return 1;
    }

    free(fatbin_buf.buf); // free fatbin buffer

    liborkh_log_info("Found %zu GPU ELF entries\n", pool->count);

    liborkh_gpu_elf_pool_iterate(pool, print_nb_kernels_in_entry, elf_basename);

    liborkh_gpu_elf_pool_free(pool);
    return 0;
}