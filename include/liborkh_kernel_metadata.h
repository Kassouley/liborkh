#ifndef LIBORKH_KERNEL_METADATA_H
#define LIBORKH_KERNEL_METADATA_H

#define LIBORKH_AMDHSAMETADATA_KEY "amdhsa.kernels"
#define LIBORKH_AMDHSAMETADATA_KEY_SIZE sizeof(LIBORKH_AMDHSAMETADATA_KEY) - 1 

#include <stdint.h>
#include <stddef.h>
#include "liborkh_utils.h"
#include "liborkh_elf_utils.h"

liborkh_status_t liborkh_get_kernels_metadata(Elf* elf, uint8_t** out_metadata, size_t* out_size);
liborkh_status_t liborkh_get_number_kernels(const uint8_t* metadata, size_t metadata_size, size_t* out_num_kernels);
liborkh_status_t liborkh_get_number_kernels_in_pool(liborkh_gpu_elf_pool_t* pool, size_t* out_num_kernels);

#endif // LIBORKH_KERNEL_METADATA_H