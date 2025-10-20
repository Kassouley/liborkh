#ifndef LIBORKH_CLANG_OFFLOAD_BUNDLER_H
#define LIBORKH_CLANG_OFFLOAD_BUNDLER_H

#include <stdint.h>
#include "liborkh_utils.h"
#include "liborkh_gpu_elf_pool.h"

#define CLANG_OFFLOAD_BUNDLER_MAGIC "__CLANG_OFFLOAD_BUNDLE__"

#define CLANG_OFFLOAD_BUNDLER_MAGIC_SIZE sizeof(CLANG_OFFLOAD_BUNDLER_MAGIC) - 1

liborkh_status_t liborkh_decode_clang_offload_bundler(const uint8_t *buf, const size_t size, liborkh_gpu_elf_pool_t* pool, liborkh_entry_filter_t* filter);

#endif // LIBORKH_CLANG_OFFLOAD_BUNDLER_H