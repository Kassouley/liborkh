#ifndef LIBORKH_UNCOMPRESS_H
#define LIBORKH_UNCOMPRESS_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <zstd.h>

#include "liborkh_utils.h"

liborkh_status_t libokrh_uncompress_zlib(const uint8_t *buf, size_t compressed_size, size_t uncompressed_size, uint8_t **out_buf, size_t *out_size) {
    uint8_t *out = (uint8_t*) malloc(uncompressed_size);
    if (!out) return LIBORKH_ERROR_OUT_OF_MEMORY;

    uLongf dest_len = uncompressed_size;

    int ret = uncompress(out, &dest_len, buf, compressed_size);

    if (ret != Z_OK) {
        free(out);
        return LIBORKH_ERROR_DECOMPRESSION_FAILED;
    }

    if (dest_len != uncompressed_size) {
        liborkh_log_warn("Size mismatch after zlib decompress\n");
    }

    *out_buf = out;
    *out_size = dest_len;
    return LIBORKH_SUCCESS;
}


liborkh_status_t libokrh_uncompress_zstd(const uint8_t *buf, size_t compressed_size, size_t uncompressed_size, uint8_t **out_buf, size_t *out_size) {
    uint8_t *out = (uint8_t*) malloc(uncompressed_size);
    if (!out) return LIBORKH_ERROR_OUT_OF_MEMORY;

    size_t ret = ZSTD_decompress(out, uncompressed_size, buf, compressed_size);

    if (ZSTD_isError(ret)) {
        liborkh_log_warn("ZSTD error: %s\n", ZSTD_getErrorName(ret));
        free(out);
        return LIBORKH_ERROR_DECOMPRESSION_FAILED;
    }

    *out_buf = out;
    *out_size = ret;
    return LIBORKH_SUCCESS;

}

#endif // LIBORKH_UNCOMPRESS_H