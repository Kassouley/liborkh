#ifndef LIBORKH_IO_H
#define LIBORKH_IO_H

#include "liborkh_utils.h"
#include "liborkh.h"

#define liborkh_open_file(file, mode, out_fp) \
    do { \
        FILE* __f = fopen(file, mode); \
        if (!__f) { \
            liborkh_log_err("Failed to open file: %s\n", file); \
            return LIBORKH_ERROR_OPEN_FILE; \
        } \
        *(out_fp) = __f; \
    } while (0)


#define liborkh_write_to_file(file, data, size) \
    do { \
        size_t __written = fwrite(data, 1, size, file); \
        if (__written != size) { \
            liborkh_log_err("Failed to write data to file\n"); \
            fclose(file); \
            return LIBORKH_ERROR_WRITE_FILE; \
        } \
    } while (0)


char* liborkh_get_elf_name(const liborkh_gpu_elf_entry_t* entry, const char* prefix);
liborkh_status_t liborkh_write_elf_to_file(const liborkh_gpu_elf_entry_t* entry, const char* prefix);
liborkh_status_t liborkh_write_fatbin_to_file(const liborkh_offload_buffer* buf, const char* filename);

#endif // LIBORKH_IO_H