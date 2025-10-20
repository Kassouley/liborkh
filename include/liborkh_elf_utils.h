#ifndef LIBORKH_ELF_UTILS_H
#define LIBORKH_ELF_UTILS_H

#include <gelf.h>
#include <libelf.h>
#include <stdint.h>

#include "liborkh_utils.h"

typedef struct {
    uint32_t namesz;
    uint32_t descsz;
    uint32_t type;
    char* name;
    uint8_t* desc;
} elf_note_t;


liborkh_status_t liborkh_open_elf(const char* filename, Elf **elf);
liborkh_status_t liborkh_open_elf_from_memory(const uint8_t* buf, size_t size, Elf **elf);
liborkh_status_t liborkh_close_elf(Elf *elf);
liborkh_status_t liborkh_get_note_data(Elf *elf, elf_note_t* out);
liborkh_status_t liborkh_extract_section_data(Elf *elf, const char* section_name, uint8_t** out_data, size_t* out_size);

#endif // LIBORKH_ELF_UTILS_H