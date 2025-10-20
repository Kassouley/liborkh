#include <stdio.h>
#include <stdlib.h>
#include <libgen.h> 
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

    char *elf_basename = basename(elf_filename);

    liborkh_close_elf(elf);

    liborkh_gpu_elf_pool_t *pool = NULL;
    if (liborkh_gpu_elf_pool_init(&pool, 4) != 0) {
        free(fatbin_buf.buf);
        return 1;
    }

    if (liborkh_get_gpu_elfs(&fatbin_buf, pool, NULL) != 0) {
        free(fatbin_buf.buf);
        return 1;
    }

    free(fatbin_buf.buf); // free fatbin buffer

    liborkh_log_info("Found %zu GPU ELF entries\n", pool->count);

    liborkh_gpu_elf_pool_iterate(pool, (liborkh_gpu_elf_pool_iterate_cb_t) liborkh_write_elf_to_file, elf_basename);

    liborkh_gpu_elf_pool_free(pool);
    return 0;
}
