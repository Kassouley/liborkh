#include <stdio.h>
#include <stdlib.h>
#include "liborkh.h"


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

    size_t total_kernels = 0;
    if (liborkh_get_number_kernels_in_pool(pool, &total_kernels) != 0) {
        return 1;
    }
    liborkh_log_info("Total number of kernels across all GPU ELF entries: %zu\n", total_kernels);

    liborkh_gpu_elf_pool_free(pool);
    return 0;
}