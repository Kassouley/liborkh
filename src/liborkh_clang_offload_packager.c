#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "liborkh_gpu_elf_pool.h"
#include "liborkh_utils.h"
#include "liborkh_clang_offload_packager.h"

liborkh_status_t liborkh_decode_clang_offload_packager(const uint8_t *buf, const size_t size, liborkh_gpu_elf_pool_t* pool, liborkh_entry_filter_t* filter)
{
    LIBORKH_CHECK_ARGUMENTS(!buf || !pool || size == 0);
    size_t pos = 0;

    while (pos + sizeof(__liborkh_offload_binary_header_t) <= size) {
        __liborkh_offload_binary_header_t *hdr = (__liborkh_offload_binary_header_t *)(buf + pos);
        
        if (hdr->magic != CLANG_OFFLOAD_PACKAGER_MAGIC) {
            pos++;
            continue; // scan forward for next possible header
        }

        if (hdr->version != CLANG_OFFLOAD_PACKAGER_HEADER_VERSION) {
            liborkh_log_warn("Unsupported version: %u\n", hdr->version);
            pos += 4;
            continue;
        }

        LIBORKH_CHECK_CALL(check_bounds(pos, hdr->size, size), "Corrupted header (out of bounds)\n");

        // Found one valid binary blob
        size_t blob_start = pos;
        size_t blob_end   = pos + hdr->size;

        // Locate offload entry table
        LIBORKH_CHECK_CALL(check_bounds(0, hdr->entry_offset + hdr->entry_size, hdr->size), "Invalid entry table bounds\n");
    
        __liborkh_offload_entry_t *entry = (__liborkh_offload_entry_t *)(buf + blob_start + hdr->entry_offset);

        LIBORKH_CHECK_CALL(check_bounds(blob_start, hdr->entry_offset + sizeof(*entry), blob_end), "Entry header out of bounds\n");
 
        if (entry->image_size == 0) {
            pos = blob_end;
            continue;
        }

        liborkh_gpu_elf_entry_t *out = NULL;
        LIBORKH_CHECK_CALL(liborkh_new_entry(&out), "Failed to get new entry\n");
     
        out->id = 0;
        out->img = (image_kind_t) entry->image_kind;
        out->ofk = (offload_kind_t) entry->offload_kind;

        // Extract target ID from string table (look for key == "triple" or "arch")
        if (entry->num_strings > 0) {
            size_t str_table_off = blob_start + entry->string_offset;
            __liborkh_offload_string_entry_t *str_entries = (__liborkh_offload_string_entry_t *)(buf + str_table_off);
            for (uint64_t i = 0; i < entry->num_strings; i++) {
                size_t key_off = blob_start + str_entries[i].key_offset;
                size_t val_off = blob_start + str_entries[i].value_offset;
                
                if (check_bounds(key_off, 1, size) == LIBORKH_SUCCESS 
                        && check_bounds(val_off, 1, size) == LIBORKH_SUCCESS) {
                    const char *key = (char *)(buf + key_off);
                    const char *val = (char *)(buf + val_off);
                    if (strcmp(key, "triple") == 0) {
                        out->target_triple_size = strlen(val);
                        out->target_triple = malloc(out->target_triple_size + 1);
                        if (out->target_triple)
                            strcpy(out->target_triple, val);
                    } else if (strcmp(key, "arch") == 0) {
                        out->target_arch_size = strlen(val);
                        out->target_arch = malloc(out->target_arch_size + 1);
                        if (out->target_arch)
                            strcpy(out->target_arch, val);
                    }
                }
            }
        }

        if (liborkh_is_entry_matching_filter(out, filter)) {
            // Extract ELF image
            size_t image_off = blob_start + entry->image_offset;
            if (check_bounds(image_off, entry->image_size, size) == LIBORKH_SUCCESS) {
                out->elf_size = entry->image_size;
                out->elf = malloc(out->elf_size);
                if (out->elf) {
                    memcpy(out->elf, buf + image_off, out->elf_size);
                    LIBORKH_CHECK_CALL(liborkh_gpu_elf_pool_push(pool, out), "Failed to add entry to pool\n");

                    if (liborkh_is_one_by_id_mode_filter(filter)) {
                        break; // only one entry requested
                    }
                }
            }
        } else {
            LIBORKH_CHECK_CALL(liborkh_free_entry(out), "Failed to free entry\n");
        }

        pos = blob_end;
    }

    return LIBORKH_SUCCESS;
}