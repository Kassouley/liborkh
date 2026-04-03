#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "liborkh.h"
#include "liborkh_gpu_elf_pool.h"
#include "liborkh_utils.h"
#include "liborkh_clang_offload_bundler.h"

static liborkh_status_t __liborkh_parse_entry_id(const uint8_t* buf, const size_t id_len, liborkh_gpu_elf_entry_t* out) 
{
    LIBORKH_CHECK_ARGUMENTS(!buf || !out || id_len == 0);

    char *entry_id = (char *) malloc(id_len + 1);
    LIBORKH_CHECK_ALLOC(entry_id);

    memcpy(entry_id, buf, id_len);
    entry_id[id_len] = '\0';

    out->ofk = OFK_None;
    out->target_triple_size = 0;
    out->target_triple = NULL;
    out->target_arch_size = 0;
    out->target_arch = NULL;

    char *first_dash = strchr(entry_id, '-');
    if (!first_dash) {
        free(entry_id);
        return LIBORKH_ERROR_CHAR_NOT_FOUND;
    }

    size_t kind_len = first_dash - entry_id;
    out->ofk = liborkh_string_to_offload_kind(entry_id, kind_len);

    // Find "--" separator (triple vs arch)
    char *double_dash = strstr(first_dash + 1, "--");
    if (!double_dash) {
        free(entry_id);
        return LIBORKH_ERROR_CHAR_NOT_FOUND;
    }

    // Extract triple
    size_t triple_len = double_dash - (first_dash + 1);
    if (triple_len > 0) {
        out->target_triple = malloc(triple_len + 1);
        LIBORKH_CHECK_ALLOC(out->target_triple);

        memcpy(out->target_triple, first_dash + 1, triple_len);
        out->target_triple[triple_len] = '\0';
        out->target_triple_size = triple_len;
    }

    // Extract arch
    const char *arch_start = double_dash + 2;
    size_t arch_len = strlen(arch_start);
    if (arch_len > 0) {
        out->target_arch = malloc(arch_len + 1);
        LIBORKH_CHECK_ALLOC(out->target_arch);
   
        memcpy(out->target_arch, arch_start, arch_len);
        out->target_arch[arch_len] = '\0';
        out->target_arch_size = arch_len;
    }

    free(entry_id);
    return LIBORKH_SUCCESS;
}

// https://rocm.docs.amd.com/projects/llvm-project/en/latest/LLVM/clang/html/ClangOffloadBundler.html#id19
liborkh_status_t liborkh_decode_compress_clang_offload_bundler_metadata(const uint8_t *buf, const size_t size, size_t* pos) 
{
    if (memcmp(buf + *pos, COMPRESSION_CLANG_OFFLOAD_BUNDLER_MAGIC, COMPRESSION_CLANG_OFFLOAD_BUNDLER_MAGIC_SIZE) == 0) {
        *pos += COMPRESSION_CLANG_OFFLOAD_BUNDLER_MAGIC_SIZE;
        uint16_t version = *(uint16_t *)(buf + *pos);
        *pos += sizeof(uint16_t);
        uint16_t compression_type = *(uint16_t *)(buf + *pos);
        *pos += sizeof(uint16_t);
        (*pos)++;
        uint64_t compressed_size = 0;
        uint64_t uncompressed_size = 0;
        if (version == 2) {
            compressed_size = *(uint32_t *)(buf + *pos);
            *pos += sizeof(uint32_t);
            uncompressed_size = *(uint32_t *)(buf + *pos);
            *pos += sizeof(uint32_t);
        } else if (version == 3) {
            compressed_size = *(uint64_t *)(buf + *pos);
            *pos += sizeof(uint64_t);
            uncompressed_size = *(uint64_t *)(buf + *pos);
            *pos += sizeof(uint64_t);
        } else {
            liborkh_log_warn("Unknown compressed bundle version %u. Skipping entry.\n", version);
        }
        uint64_t hash = *(uint64_t *)(buf + *pos);
        *pos += sizeof(uint64_t);

        liborkh_log_warn("Compressed code objects are not yet supported (version %u, method %u, hash %lx). Skipping entry.\n", version, compression_type, (unsigned long)hash);
        *pos += compressed_size; // skip compressed data
        return LIBORKH_ERROR_UNKNOWN;

    }
    return LIBORKH_SUCCESS;
}

liborkh_status_t liborkh_decode_clang_offload_bundler(const uint8_t *buf, const size_t size, liborkh_gpu_elf_pool_t* pool, liborkh_entry_filter_t* filter)
{
    LIBORKH_CHECK_ARGUMENTS(!buf || !pool || size == 0);

    size_t pos = 0;
    size_t last_elf_end = 1;
    size_t bundle_count = 0;

    while (pos + CLANG_OFFLOAD_BUNDLER_MAGIC_SIZE <= size) {
        if (liborkh_decode_compress_clang_offload_bundler_metadata(buf, size, &pos) != LIBORKH_SUCCESS) {
            continue;
        }

        // Find next bundle magic
        if (memcmp(buf + pos, CLANG_OFFLOAD_BUNDLER_MAGIC, CLANG_OFFLOAD_BUNDLER_MAGIC_SIZE) != 0) {
            pos++;
            continue;
        }

        size_t bundle_start = pos;
        pos += CLANG_OFFLOAD_BUNDLER_MAGIC_SIZE;

        LIBORKH_CHECK_CALL(check_bounds(pos, sizeof(uint64_t), size), "Truncated bundle header (no entry count)\n");

        // Read number of entries
        uint64_t num_entries = *(uint64_t *)(buf + pos);
        pos += sizeof(uint64_t);

        for (uint64_t i = 0; i < num_entries; i++) {
            // Check metadata bounds
            LIBORKH_CHECK_CALL(check_bounds(pos, sizeof(uint64_t) * 3, size), "Truncated metadata for entry %llu\n", i);

            uint64_t elf_offset = *(uint64_t *)(buf + pos); pos += sizeof(uint64_t);
            uint64_t elf_size   = *(uint64_t *)(buf + pos); pos += sizeof(uint64_t);
            uint64_t id_len     = *(uint64_t *)(buf + pos); pos += sizeof(uint64_t);

            size_t elf_start = bundle_start + elf_offset;
            LIBORKH_CHECK_CALL(check_bounds(elf_start, elf_size, size), "Code object %llu out of bounds\n", i);

            last_elf_end = elf_start + elf_size;
            
            if (elf_size == 0) {
                pos += id_len;
                continue;
            }

            // Validate ID string
            LIBORKH_CHECK_CALL(check_bounds(pos, id_len, size), "Invalid ID length for entry %llu\n", i);
        
            liborkh_gpu_elf_entry_t *entry = NULL;
            LIBORKH_CHECK_CALL(liborkh_new_entry(&entry), "Failed to get new entry\n");

            entry->id  = bundle_count;
            entry->img = IMG_Fatbinary;

            LIBORKH_CHECK_CALL(__liborkh_parse_entry_id(buf + pos, id_len, entry), "Failed to parse entry ID\n");

            pos += id_len;

            if (elf_size > 0 && liborkh_is_entry_matching_filter(entry, filter)) {
                // Extract code object
                entry->elf_size = elf_size;
                entry->elf = malloc(entry->elf_size);
                LIBORKH_CHECK_ALLOC(entry->elf);
              
                memcpy(entry->elf, buf + elf_start, entry->elf_size);

                // Add entry to pool
                LIBORKH_CHECK_CALL(liborkh_gpu_elf_pool_push(pool, entry), "Failed to add entry to pool\n");
                
                if (liborkh_is_one_by_id_mode_filter(filter)) {
                    break; // only one entry requested
                }
            } else {
                LIBORKH_CHECK_CALL(liborkh_free_entry(entry), "Failed to free entry\n");
            }
        }

        // Move to the end of the last bundle to continue searching for the next bundle
        pos = last_elf_end;
        bundle_count++;
    }
    return LIBORKH_SUCCESS;
}