#include <stdio.h>
#include <stdlib.h>
#include "liborkh.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <input ELF file> <output>\n", argv[0]);
        return 1;
    }

    const char *elf_filename = argv[1] ? argv[1] : "./a.out";
    const char *output       = argv[2] ? argv[2] : "output.fatbin";

    Elf *elf = NULL;
    if (liborkh_open_elf(elf_filename, &elf) != 0) {
        return 1;
    }

    
    liborkh_offload_buffer fatbin_buf = {0};
    if (liborkh_extract_gpu_fatbin(elf, &fatbin_buf) != 0) {
        liborkh_close_elf(elf);
        return 1;
    }


    liborkh_write_fatbin_to_file(&fatbin_buf, output);

    liborkh_close_elf(elf);
    return 0;
}
