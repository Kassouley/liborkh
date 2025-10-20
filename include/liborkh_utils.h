#ifndef LIBORKH_UTILS_H
#define LIBORKH_UTILS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include "liborkh_log.h"

#define LIBORKH_CHECK_CALL(call, msg, ...)       \
    do {                                         \
        liborkh_status_t __status = (call);      \
        if (__status != LIBORKH_SUCCESS) {       \
            liborkh_log_err(msg, ##__VA_ARGS__); \
            return __status;                     \
        }                                        \
    } while (0)


#define LIBORKH_CHECK_ARGUMENTS(cond)               \
    do {                                            \
        if (cond) {                                 \
            liborkh_log_err("Invalid arguments\n"); \
            return LIBORKH_ERROR_INVALID_ARGUMENT;  \
        }                                           \
    } while (0)

    
#define LIBORKH_CHECK_ALLOC(ptr)                 \
    do {                                         \
        if (!(ptr)) {                            \
            liborkh_log_err("Out of memory\n");  \
            return LIBORKH_ERROR_OUT_OF_MEMORY;  \
        }                                        \
    } while (0)


typedef enum {
    LIBORKH_SUCCESS = 0,
    LIBORKH_ERROR_INVALID_ARGUMENT,
    LIBORKH_ERROR_OUT_OF_MEMORY,
    LIBORKH_ERROR_POOL_EMPTY,
    LIBORKH_ERROR_OUT_OF_BOUNDS,
    LIBORKH_ERROR_CHAR_NOT_FOUND,
    LIBORKH_ERROR_ELF,
    LIBORKH_ERROR_OPEN_FILE,
    LIBORKH_ERROR_WRITE_FILE,
    LIBORKH_ERROR_SECTION_NOT_FOUND,
    LIBORKH_ERROR_METADATA_NOT_FOUND,
    LIBORKH_ERROR_IO,
    LIBORKH_ERROR_UNKNOWN
} liborkh_status_t;


typedef enum {
    IMG_None      = 0x00,
    IMG_Object    = 0x01,
    IMG_Bitcode   = 0x02,
    IMG_Cubin     = 0x03,
    IMG_Fatbinary = 0x04,
    IMG_PTX       = 0x05
} image_kind_t;

typedef enum {
    OFK_None   = 0x00,
    OFK_OpenMP = 0x01,
    OFK_CUDA   = 0x02,
    OFK_HIP    = 0x03,

    OFK_HOST   = 0x90,
    OFK_HIPV4  = 0x91,
} offload_kind_t;


typedef struct {
    size_t id;
    image_kind_t img;
    offload_kind_t ofk;
    size_t target_triple_size;
    char* target_triple;
    size_t target_arch_size;
    char* target_arch;
    size_t elf_size;
    uint8_t* elf;
} liborkh_gpu_elf_entry_t;


typedef enum {
    FILTER_ID_MODE_ALL_IDS = 0,
    FILTER_ID_MODE_ONE_BY_ID,
} liborkh_filter_id_mode_t;

typedef struct {
    liborkh_filter_id_mode_t id_mode;
    image_kind_t img;
    offload_kind_t ofk;
    const char* target_triple;
    const char* target_arch;
} liborkh_entry_filter_t;


static inline liborkh_status_t check_bounds(size_t offset, size_t len, size_t limit) {
    return (offset + len <= limit) ? LIBORKH_SUCCESS : LIBORKH_ERROR_OUT_OF_BOUNDS;
}

static inline bool liborkh_is_one_by_id_mode_filter(const liborkh_entry_filter_t *filter) {
    return filter && filter->id_mode == FILTER_ID_MODE_ONE_BY_ID;
}

static inline bool liborkh_is_entry_matching_filter(const liborkh_gpu_elf_entry_t *entry, const liborkh_entry_filter_t *filter)
{
    if (!entry)
        return false;
    if (!filter)
        return true;

    if ((filter->img != IMG_None && entry->img != filter->img) ||
        (filter->ofk != OFK_None && entry->ofk != filter->ofk))
        return false;

    if (filter->target_triple &&
        (!entry->target_triple || strcmp(entry->target_triple, filter->target_triple) != 0))
        return false;

    if (filter->target_arch &&
        (!entry->target_arch || strcmp(entry->target_arch, filter->target_arch) != 0))
        return false;

    return true;
}


#endif // LIBORKH_UTILS_H