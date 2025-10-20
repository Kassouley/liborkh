#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "liborkh.h"
#include "liborkh_utils.h"

liborkh_status_t liborkh_write_fatbin_to_file(const liborkh_offload_buffer* buf, const char* filename) {
    LIBORKH_CHECK_ARGUMENTS(!buf || !filename);

    FILE* f = NULL;
    liborkh_open_file(filename, "wb", &f);
    liborkh_write_to_file(f, buf->buf, buf->size);
    fclose(f);

    liborkh_log_info("Wrote fatbin to file: %s (%zu bytes)\n", filename, buf->size);
    return LIBORKH_SUCCESS;
}


char* liborkh_get_elf_name(const liborkh_gpu_elf_entry_t* entry, const char* prefix) {
    prefix = prefix ? prefix : "gpu_elf";

    // Create filename: <prefix>:<id>.<image>.<offload_kind>.<triple>.<arch>
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%s:%ld.%s.%s.%s.%s",
             prefix,
             entry->id,
             liborkh_image_kind_to_string(entry->img),
             liborkh_offload_kind_to_string(entry->ofk),
             entry->target_triple ? entry->target_triple : "unknown",
             entry->target_arch ? entry->target_arch : "unknown");

    char* filename = strdup(buffer);
    if (!filename) {
        liborkh_log_err("Failed to allocate memory for filename\n");
        return NULL;
    }

    return filename;
}


liborkh_status_t liborkh_write_elf_to_file(const liborkh_gpu_elf_entry_t* entry, const char* prefix) {
    LIBORKH_CHECK_ARGUMENTS(!entry);

    char* filename = liborkh_get_elf_name(entry, prefix);
    if (!filename) {
        return LIBORKH_ERROR_OUT_OF_MEMORY;
    }

    if (!entry->elf || entry->elf_size == 0) {
        liborkh_log_warn("Cannot write elf in %s. Entry doesn't contain any elf data.\n", filename);
        free(filename);
        return LIBORKH_SUCCESS;
    }

    FILE* f = NULL;
    liborkh_open_file(filename, "wb", &f);
    liborkh_write_to_file(f, entry->elf, entry->elf_size);
    fclose(f);

    liborkh_log_info("Wrote ELF to file: %s (%zu bytes)\n", filename, entry->elf_size);

    free(filename);
    return 0;
}