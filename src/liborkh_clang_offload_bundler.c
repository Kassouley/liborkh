#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "liborkh.h"
#include "liborkh_gpu_elf_pool.h"
#include "liborkh_utils.h"
#include "liborkh_clang_offload_bundler.h"
#include "liborkh_uncompress.h"

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
liborkh_compressed_bundle_entry_t __liborkh_decode_compress_clang_offload_bundler_metadata(const uint8_t *buf, const size_t size, size_t* pos) 
{
    liborkh_compressed_bundle_entry_t entry = {0};

    *pos += COMPRESSION_CLANG_OFFLOAD_BUNDLER_MAGIC_SIZE;
    size_t header_size = sizeof(uint16_t) * 2 + sizeof(uint64_t) + COMPRESSION_CLANG_OFFLOAD_BUNDLER_MAGIC_SIZE;
    uint16_t version = read_u16(buf, pos);
    uint16_t compression_type = read_u16(buf, pos); // 0 = zlib, 1 = zstd
    uint64_t compressed_size = 0;
    uint64_t uncompressed_size = 0;
    if (version == 2) {
        header_size += sizeof(uint32_t) * 2;
        compressed_size = read_u32(buf, pos) - header_size; // compressed size includes header
        uncompressed_size = read_u32(buf, pos);
    } else if (version == 3) {
        header_size += sizeof(uint64_t) * 2;
        compressed_size = read_u64(buf, pos) - header_size; // compressed size includes header
        uncompressed_size = read_u64(buf, pos);
    } else {
        liborkh_log_err("Unknown compressed bundle version %u. Skipping entry.\n", version);
        return entry;
    }

    uint64_t hash = read_u64(buf, pos);

    uint8_t *uncompressed_data = NULL;

    liborkh_status_t status = LIBORKH_SUCCESS;
    if (compression_type == 0) {
        status = libokrh_uncompress_zlib(buf + *pos, compressed_size, uncompressed_size, &uncompressed_data, &uncompressed_size);
    } else if (compression_type == 1) {
        status = libokrh_uncompress_zstd(buf + *pos, compressed_size, uncompressed_size, &uncompressed_data, &uncompressed_size);
    } else {
        liborkh_log_warn("Unknown compression type %u in compressed bundle. Skipping entry.\n", compression_type);
        *pos += compressed_size; // skip compressed data
        return entry;
    }

    *pos += compressed_size; // skip compressed data

    if (status != LIBORKH_SUCCESS) {
        liborkh_log_warn("Failed to uncompress bundle entry (type %u, version %u). Skipping entry.\n", compression_type, version);
        return entry;
    }
    
    entry.version = version;
    entry.compression_type = compression_type;
    entry.compressed_size = compressed_size;
    entry.uncompressed_size = uncompressed_size;
    entry.hash = hash;
    entry.uncompressed_data = uncompressed_data;

    return entry;
}

liborkh_status_t __liborkh_decode_bundle(const uint8_t *buf, const size_t size, liborkh_gpu_elf_pool_t* pool, liborkh_entry_filter_t* filter, size_t bundle_id, size_t* bundle_size) 
{
    size_t pos = 0;
    pos += CLANG_OFFLOAD_BUNDLER_MAGIC_SIZE;

    LIBORKH_CHECK_CALL(check_bounds(pos, sizeof(uint64_t), size), "Truncated bundle header (no entry count)\n");

    // Read number of entries
    uint64_t num_entries = read_u64(buf, &pos);

    for (uint64_t i = 0; i < num_entries; i++) {
        // Check metadata bounds
        LIBORKH_CHECK_CALL(check_bounds(pos, sizeof(uint64_t) * 3, size), "Truncated metadata for entry %llu\n", i);

        uint64_t elf_start  = read_u64(buf, &pos);
        uint64_t elf_size   = read_u64(buf, &pos);
        uint64_t id_len     = read_u64(buf, &pos);

        LIBORKH_CHECK_CALL(check_bounds(elf_start, elf_size, size), "Code object %llu out of bounds\n", i);

        *bundle_size = elf_start + elf_size;
        
        if (elf_size == 0) {
            pos += id_len;
            continue;
        }

        // Validate ID string
        LIBORKH_CHECK_CALL(check_bounds(pos, id_len, size), "Invalid ID length for entry %llu\n", i);
    
        liborkh_gpu_elf_entry_t *entry = NULL;
        LIBORKH_CHECK_CALL(liborkh_new_entry(&entry), "Failed to get new entry\n");

        entry->id  = bundle_id;
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
    return LIBORKH_SUCCESS;
}

liborkh_status_t liborkh_decode_clang_offload_bundler(const uint8_t *buf, const size_t size, liborkh_gpu_elf_pool_t* pool, liborkh_entry_filter_t* filter)
{
    LIBORKH_CHECK_ARGUMENTS(!buf || !pool || size == 0);

    size_t pos = 0;
    size_t bundle_size = 1;
    size_t bundle_count = 0;
    while (pos <= size) {
        bundle_size = 1;
        if (is_magic(buf, pos, COMPRESSION_CLANG_OFFLOAD_BUNDLER_MAGIC)) {
            liborkh_compressed_bundle_entry_t entry = __liborkh_decode_compress_clang_offload_bundler_metadata(buf, size, &pos);
            if (entry.uncompressed_data) {
                liborkh_status_t status = __liborkh_decode_bundle(entry.uncompressed_data, entry.uncompressed_size, pool, filter, bundle_count, &bundle_size);
                free(entry.uncompressed_data);
                if (status != LIBORKH_SUCCESS) {
                    liborkh_log_warn("Failed to decode compressed bundle entry %zu\n", bundle_count);
                }
            }
            bundle_size = 1; // pos has already been advanced in the compressed bundle metadata function and bundle_size doesn't correspond to the compressed data size
        } else if (is_magic(buf, pos, CLANG_OFFLOAD_BUNDLER_MAGIC)) {
            liborkh_status_t status = __liborkh_decode_bundle(buf + pos, size - pos, pool, filter, bundle_count, &bundle_size);
            if (status != LIBORKH_SUCCESS) {
                liborkh_log_warn("Failed to decode bundle entry %zu\n", bundle_count);
            }
        } else {
            pos++;
            continue; // not a bundle, keep searching
        }
        pos += bundle_size;
        bundle_count++;
    }
    return LIBORKH_SUCCESS;
}