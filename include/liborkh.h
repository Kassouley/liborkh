#ifndef LIBORKH_H
#define LIBORKH_H

#include <stdint.h>

#include "liborkh_utils.h"
#include "liborkh_elf_utils.h"
#include "liborkh_log.h"
#include "liborkh_gpu_elf_pool.h"
#include "liborkh_clang_offload_packager.h"
#include "liborkh_clang_offload_bundler.h"
#include "liborkh_kernel_metadata.h"

#define LIBORKH_HIP_FATBIN_SECTION_NAME             ".hip_fatbin"
#define LIBORKH_LLVM_OFFLOADING_FATBIN_SECTION_NAME ".llvm.offloading"

typedef enum {
    UNKNOWN_KIND,
    CLANG_OFFLOAD_PACKAGER_KIND,
    CLANG_OFFLOAD_BUNDLER_KIND
} liborkh_offload_encoding_kind;

typedef struct {
    liborkh_offload_encoding_kind kind;
    uint8_t *buf;
    size_t size;
} liborkh_offload_buffer;

#include "liborkh_io.h"

liborkh_status_t liborkh_get_gpu_elfs(liborkh_offload_buffer *buf, liborkh_gpu_elf_pool_t *pool, liborkh_entry_filter_t* filter);
liborkh_status_t liborkh_extract_gpu_fatbin(Elf *elf, liborkh_offload_buffer *out);

const char* liborkh_offload_kind_to_string(offload_kind_t ofk);
offload_kind_t liborkh_string_to_offload_kind(const char *str, const size_t len);
const char* liborkh_image_kind_to_string(image_kind_t img);

#endif // LIBORKH_H