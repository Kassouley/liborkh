#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "liborkh.h"

liborkh_status_t liborkh_get_gpu_elfs(liborkh_offload_buffer *buf, liborkh_gpu_elf_pool_t *pool, liborkh_entry_filter_t* filter) {
    LIBORKH_CHECK_ARGUMENTS(!buf || !pool);

    switch (buf->kind) {
        case CLANG_OFFLOAD_BUNDLER_KIND:  return liborkh_decode_clang_offload_bundler(buf->buf, buf->size, pool, filter);
        case CLANG_OFFLOAD_PACKAGER_KIND: return liborkh_decode_clang_offload_packager(buf->buf, buf->size, pool, filter);
        default: liborkh_log_warn("Unknown offload kind : %d\n", buf->kind); break;
    }
    return LIBORKH_SUCCESS;
}

liborkh_status_t liborkh_extract_gpu_fatbin(Elf *elf, liborkh_offload_buffer *out) {
    LIBORKH_CHECK_ARGUMENTS(!elf || !out);

    size_t shstrndx = 0;
    Elf_Scn *scn = NULL;
    GElf_Shdr shdr;

    out->kind = UNKNOWN_KIND;
    out->buf  = NULL;
    out->size = 0;
    
    if (elf_getshdrstrndx(elf, &shstrndx) != 0) {
        liborkh_log_err("elf_getshdrstrndx() failed: %s\n", elf_errmsg(-1));
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

        if (strcmp(name, LIBORKH_HIP_FATBIN_SECTION_NAME) == 0) {
            out->kind = CLANG_OFFLOAD_BUNDLER_KIND;
        } else if (strcmp(name, LIBORKH_LLVM_OFFLOADING_FATBIN_SECTION_NAME) == 0) {
            out->kind = CLANG_OFFLOAD_PACKAGER_KIND;
        } else {
            continue;
        }

        Elf_Data *data = elf_getdata(scn, NULL);
        if (!data) {
            liborkh_log_err("elf_getdata() failed: %s\n", elf_errmsg(-1));
            return LIBORKH_ERROR_ELF;
        } 

        unsigned char *buf = malloc(data->d_size);
        LIBORKH_CHECK_ALLOC(buf);

        memcpy(buf, data->d_buf, data->d_size);
        out->buf  = buf;
        out->size = data->d_size;
        break;
    }
    return LIBORKH_SUCCESS;
}

const char* liborkh_image_kind_to_string(image_kind_t img) {
    switch (img) {
        case IMG_None:      return "none";
        case IMG_Object:    return "object";
        case IMG_Bitcode:   return "bitcode";
        case IMG_Cubin:     return "cubin";
        case IMG_Fatbinary: return "fatbin";
        case IMG_PTX:       return "ptx";
        default:            return "unknown";
    }
}

const char* liborkh_offload_kind_to_string(offload_kind_t ofk) {
    switch (ofk) {
        case OFK_None:   return "none";
        case OFK_OpenMP: return "openmp";
        case OFK_CUDA:   return "cuda";
        case OFK_HIP:    return "hip";
        case OFK_HOST:   return "host";
        case OFK_HIPV4:  return "hipv4";
        default:         return "unknown";
    }
}

offload_kind_t liborkh_string_to_offload_kind(const char *str, const size_t len) {
    if (len == 4 && strncmp(str, "none",   len) == 0) return OFK_None;
    if (len == 6 && strncmp(str, "openmp", len) == 0) return OFK_OpenMP;
    if (len == 4 && strncmp(str, "cuda",   len) == 0) return OFK_CUDA;
    if (len == 3 && strncmp(str, "hip",    len) == 0) return OFK_HIP;
    if (len == 4 && strncmp(str, "host",   len) == 0) return OFK_HOST;
    if (len == 5 && strncmp(str, "hipv4",  len) == 0) return OFK_HIPV4;
    return OFK_None;
}
