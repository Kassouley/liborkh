#ifndef LIBORKH_CLANG_OFFLOAD_PACKAGER_H
#define LIBORKH_CLANG_OFFLOAD_PACKAGER_H

#include <stdint.h>
#include "liborkh_utils.h"
#include "liborkh_gpu_elf_pool.h"

#define CLANG_OFFLOAD_PACKAGER_MAGIC 0xAD10FF10
#define CLANG_OFFLOAD_PACKAGER_HEADER_VERSION 1

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint64_t size;
    uint64_t entry_offset;
    uint64_t entry_size;
} __liborkh_offload_binary_header_t;

typedef struct __attribute__((packed)) {
    uint16_t image_kind;
    uint16_t offload_kind;
    uint32_t flags;
    uint64_t string_offset;
    uint64_t num_strings;
    uint64_t image_offset;
    uint64_t image_size;
} __liborkh_offload_entry_t;

typedef struct __attribute__((packed)) {
    uint64_t key_offset;
    uint64_t value_offset;
} __liborkh_offload_string_entry_t;


liborkh_status_t liborkh_decode_clang_offload_packager(const uint8_t *buf, const size_t size, liborkh_gpu_elf_pool_t* pool, liborkh_entry_filter_t* filter);

#endif // LIBORKH_CLANG_OFFLOAD_PACKAGER_H