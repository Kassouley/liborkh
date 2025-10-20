#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>

#include "liborkh_utils.h"
#include "liborkh_elf_utils.h"
#include "liborkh_log.h"

liborkh_status_t liborkh_open_elf(const char* filename, Elf **elf) {
    LIBORKH_CHECK_ARGUMENTS(!filename || !elf);
   
    int fd = -1;

    if (elf_version(EV_CURRENT) == EV_NONE) {
        liborkh_log_err("ELF library initialization failed: %s\n", elf_errmsg(-1));
        return LIBORKH_ERROR_ELF;
    }

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        liborkh_log_err("Failed to open file: %s\n", filename);
        return LIBORKH_ERROR_OPEN_FILE;
    }

    *elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!*elf) {
        liborkh_log_err("elf_begin() failed: %s\n", elf_errmsg(-1));
        close(fd);
        return LIBORKH_ERROR_ELF;
    }

    return LIBORKH_SUCCESS;
}

liborkh_status_t liborkh_open_elf_from_memory(const uint8_t* buf, size_t size, Elf **elf) {
    LIBORKH_CHECK_ARGUMENTS(!elf || !buf || size == 0);

    if (elf_version(EV_CURRENT) == EV_NONE) {
        liborkh_log_err("ELF library initialization failed: %s\n", elf_errmsg(-1));
        return LIBORKH_ERROR_ELF;
    }

    *elf = elf_memory((char*)buf, size);
    if (!*elf) {
        liborkh_log_err("elf_memory() failed: %s\n", elf_errmsg(-1));
        return LIBORKH_ERROR_ELF;
    }

    if (elf_kind(*elf) != ELF_K_ELF) {
        liborkh_log_err("Not an ELF object\n");
        elf_end(*elf);
        return LIBORKH_ERROR_ELF;
    }

    return LIBORKH_SUCCESS;
}

liborkh_status_t liborkh_close_elf(Elf *elf) {
    LIBORKH_CHECK_ARGUMENTS(!elf);

    int fd = elf_getbase(elf);
    elf_end(elf);
    if (fd >= 0) close(fd);
    return LIBORKH_SUCCESS;
}


/**
 * Extracts the first note from a section and fills the elf_note_t struct.
 */
liborkh_status_t liborkh_get_note_data(Elf *elf, elf_note_t* out) {
    LIBORKH_CHECK_ARGUMENTS(!elf || !out);

    uint8_t *data = NULL;
    size_t size = 0;

    // Extract the .note section
    LIBORKH_CHECK_CALL(liborkh_extract_section_data(elf, ".note", &data, &size), "Failed to extract .note section\n");

    size_t pos = 0;

    // Read the note header
    if (size < sizeof(uint32_t) * 3) return -1; // namesz + descsz + type = 12 bytes
    out->namesz = *(uint32_t*)(data + pos); pos += sizeof(uint32_t);
    out->descsz = *(uint32_t*)(data + pos); pos += sizeof(uint32_t);
    out->type   = *(uint32_t*)(data + pos); pos += sizeof(uint32_t);

    // Ensure there is enough data for name and desc
    LIBORKH_CHECK_CALL(check_bounds(pos, out->namesz + out->descsz, size), "Truncated note data\n");

    // Allocate and copy the name
    out->name = (char*)malloc(out->namesz + 1);
    LIBORKH_CHECK_ALLOC(out->name);

    memcpy(out->name, data + pos, out->namesz);
    out->name[out->namesz] = '\0'; // Ensure null-terminated
    pos += ((out->namesz + 3) & ~3); // Align to 4 bytes

    // Allocate and copy the descriptor
    out->desc = (uint8_t*)malloc(out->descsz);
    LIBORKH_CHECK_ALLOC(out->desc);
  
    memcpy(out->desc, data + pos, out->descsz);
    pos += ((out->descsz + 3) & ~3); // Align to 4 bytes

    // Success
    return LIBORKH_SUCCESS;
}

liborkh_status_t liborkh_extract_section_data(Elf *elf, const char* section_name, uint8_t** out_data, size_t* out_size) {
    LIBORKH_CHECK_ARGUMENTS(!elf || !section_name || !out_data || !out_size);

    size_t shstrndx = 0;
    Elf_Scn *scn = NULL;
    GElf_Shdr shdr;

    int found = 0;

    *out_data = NULL;
    *out_size = 0;

    if (elf_getshdrstrndx(elf, &shstrndx) != 0) {
        liborkh_log_err("elf_getshdrstrndx() failed: %s\n", elf_errmsg(-1));
        elf_end(elf);
        return LIBORKH_ERROR_ELF;
    }

    // Iterate through sections
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        if (gelf_getshdr(scn, &shdr) != &shdr) {
            liborkh_log_err("gelf_getshdr() failed: %s\n", elf_errmsg(-1));
            return LIBORKH_ERROR_ELF;
        }

        const char *name = elf_strptr(elf, shstrndx, shdr.sh_name);
        if (!name) { 
            liborkh_log_err("elf_strptr() failed: %s\n", elf_errmsg(-1));
            return LIBORKH_ERROR_ELF;
        }

        if (strcmp(name, section_name) == 0) {
            Elf_Data *data = elf_getdata(scn, NULL);
            if (!data) {
                liborkh_log_err("elf_getdata() failed: %s\n", elf_errmsg(-1));
                return LIBORKH_ERROR_ELF;
            } 

            unsigned char *buf = malloc(data->d_size);
            LIBORKH_CHECK_ALLOC(buf);
        
            memcpy(buf, data->d_buf, data->d_size);
            *out_data = buf;
            *out_size = data->d_size;
            found = 1;
            break;
        }
    }
    if (!found) {
        liborkh_log_err("Section %s not found\n", section_name);
        return LIBORKH_ERROR_SECTION_NOT_FOUND;
    }
    return LIBORKH_SUCCESS;
}
