#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>

#include "liborkh.h"

// -------------- Helper functions to parse Lua table into filter struct --------------
/**
 * Convert integer field from table safely
 * @param L Lua state
 * @param key Field key of the lua table
 * @param def Default value if field not found or not a number
 * @return Integer value at the wanted key or default
 */
static int get_int_field(lua_State* L, const char* key, int def) {
    int val = def;
    lua_getfield(L, -1, key);
    if (lua_isnumber(L, -1)) {
        val = (int)lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
    return val;
}


/**
 * Convert string field from table safely
 * @param L Lua state
 * @param key Field key of the lua table
 * @return String value at the wanted key or NULL
 */
static const char* get_str_field(lua_State* L, const char* key) {
    const char* val = NULL;
    lua_getfield(L, -1, key);
    if (lua_isstring(L, -1)) {
        val = lua_tostring(L, -1);
    }
    lua_pop(L, 1);
    return val;
}

/**
 * Parse filter from Lua table
 * @param L Lua state
 * @param index Index of the table in the stack
 * @param filter Output filter structure
 */
static void parse_filter_from_table(lua_State* L, int index, liborkh_entry_filter_t* filter) {
    // Default values
    filter->id_mode       = FILTER_ID_MODE_ALL_IDS;
    filter->img           = IMG_None;
    filter->ofk           = OFK_None;
    filter->target_triple = NULL;
    filter->target_arch   = NULL;

    if (!lua_istable(L, index)) {
        return;
    }

    luaL_checktype(L, index, LUA_TTABLE);
    lua_pushvalue(L, index);  // push table to top of stack

    filter->id_mode       = (liborkh_filter_id_mode_t)get_int_field(L, "id_mode", filter->id_mode);
    filter->img           = (image_kind_t)get_int_field(L, "img", filter->img);
    filter->ofk           = (offload_kind_t)get_int_field(L, "ofk", filter->ofk);
    filter->target_triple = get_str_field(L, "target_triple");
    filter->target_arch   = get_str_field(L, "target_arch");

    lua_pop(L, 1);  // pop table
}



// -------------- liborkh helper functions --------------

/**
 * Get offload buffer from ELF file
 * Call all the necessary liborkh functions to open ELF and extract fatbin
 * @param L Lua state
 * @param elf_filename Path to ELF file
 * @param out_buf Output offload buffer
 * @return 0 on success, else lua error
 */
static int get_offload_buffer(lua_State* L, const char* elf_filename, liborkh_offload_buffer* out_buf) {

    Elf *elf = NULL;
    if (liborkh_open_elf(elf_filename, &elf) != 0) {
        return luaL_error(L, "failed to open ELF file: %s", elf_filename);
    }

    if (liborkh_extract_gpu_fatbin(elf, out_buf) != 0) {
        liborkh_close_elf(elf);
        return luaL_error(L, "failed to extract GPU fatbin from ELF file: %s", elf_filename);
    }

    liborkh_close_elf(elf);
    return 0;
}


/**
 * Get GPU ELF pool from offload buffer
 * Call all the necessary liborkh functions to parse fatbin and extract GPU ELFs
 * @param L Lua state
 * @param fatbin_buf Offload buffer containing fatbin
 * @param filter Entry filter
 * @param out_pool Output GPU ELF pool
 * @return 0 on success, else lua error
 */
static int get_gpu_elf_pool(lua_State* L, liborkh_offload_buffer* fatbin_buf, liborkh_entry_filter_t* filter, liborkh_gpu_elf_pool_t** out_pool) {

    liborkh_gpu_elf_pool_t *pool = NULL;
    if (liborkh_gpu_elf_pool_init(&pool, 4) != 0) {
        free(fatbin_buf->buf);
        return luaL_error(L, "failed to initialize GPU ELF pool");
    }

    if (liborkh_get_gpu_elfs(fatbin_buf, pool, filter) != 0) {
        free(fatbin_buf->buf);
        liborkh_gpu_elf_pool_free(pool);
        return luaL_error(L, "failed to get GPU ELFs");
    }

    free(fatbin_buf->buf);

    *out_pool = pool;
    return 0;
}


// -------------- Lua binding functions --------------

/**
 * Get number of kernels in ELF file matching filter
 * Lua arguments:
 * 1. elf_filename (string): Path to ELF file
 * 2. filter (table or nil): Filter table
 * Lua returns:
 * 1. number of kernels (integer)
 */
static int l_get_kernel_count(lua_State *L) {
    size_t total_kernels = 0;

    int status = 0;
    const char *elf_filename = luaL_checkstring(L, 1);
    liborkh_entry_filter_t filter = {0};
    parse_filter_from_table(L, 2, &filter);

    liborkh_offload_buffer fatbin_buf = {0};
    status = get_offload_buffer(L, elf_filename, &fatbin_buf);
    if (status != 0) {
        return status;
    }

    if (fatbin_buf.kind != UNKNOWN_KIND) {
        liborkh_gpu_elf_pool_t *pool = NULL;
        status = get_gpu_elf_pool(L, &fatbin_buf, &filter, &pool);;
        if (status != 0) {
            return status;
        }

        if (liborkh_get_number_kernels_in_pool(pool, &total_kernels) != 0) {
            liborkh_gpu_elf_pool_free(pool);
            return luaL_error(L, "failed to get number of kernels in pool");
        }

        liborkh_gpu_elf_pool_free(pool);
    }

    lua_pushinteger(L, (lua_Integer)total_kernels);
    return 1;
}


/**
 * Context structure for metadata callback
 */
typedef struct {
    lua_State *L;
    int table_index;
    const char *elf_filename;
} metadata_callback_ctx_t;


/**
 * Callback to set metadata buffer in Lua table
 */
static liborkh_status_t set_metadata_buffer(liborkh_gpu_elf_entry_t *entry, void *user_data) {
    metadata_callback_ctx_t *ctx = (metadata_callback_ctx_t *)user_data;
    lua_State *L = ctx->L;

    uint8_t *metadata = NULL;
    size_t metadata_size = 0;
    Elf * gpu_elf = NULL;

    char* gpu_elf_name = liborkh_get_elf_name(entry, ctx->elf_filename);

    if (!entry->elf || entry->elf_size == 0) {
        free(gpu_elf_name);
        return LIBORKH_SUCCESS;
    }

    liborkh_status_t status = LIBORKH_SUCCESS;

    status = liborkh_open_elf_from_memory(entry->elf, entry->elf_size, &gpu_elf);
    if (status != LIBORKH_SUCCESS) {
        luaL_error(L, "Cannot open ELF from memory for entry %s\n", gpu_elf_name);
        free(gpu_elf_name);
        return status;
    }

    status = liborkh_get_kernels_metadata(gpu_elf, &metadata, &metadata_size);
    if (status != LIBORKH_SUCCESS) {
        liborkh_close_elf(gpu_elf);
        luaL_error(L, "Cannot get kernel metadata from ELF in entry %s\n", gpu_elf_name);
        free(gpu_elf_name);
        return status;
    }

    liborkh_close_elf(gpu_elf);

    lua_pushstring(L, gpu_elf_name);

    lua_newtable(L);
    lua_pushlightuserdata(L, metadata);
    lua_setfield(L, -2, "data");
    lua_pushinteger(L, metadata_size);
    lua_setfield(L, -2, "size");

    lua_settable(L, ctx->table_index);

    free(gpu_elf_name);

    return status;
}

/**
 * Get metadata buffers for kernels in ELF file matching filter
 * Lua arguments:
 * 1. elf_filename (string): Path to ELF file
 * 2. filter (table or nil): Filter table
 * Lua returns:
 * 1. table mapping GPU ELF names to metadata buffers and sizes
 */
static int l_get_metadata_buffer(lua_State* L) {
    int status = 0;
    const char *elf_filename = luaL_checkstring(L, 1);

    liborkh_entry_filter_t filter = {0};
    parse_filter_from_table(L, 2, &filter);


    liborkh_offload_buffer fatbin_buf = {0};
    status = get_offload_buffer(L, elf_filename, &fatbin_buf);
    if (status != 0) {
        return status;
    }

    if (fatbin_buf.kind != UNKNOWN_KIND) {
        return 0;
    }
    
    liborkh_gpu_elf_pool_t *pool = NULL;
    status = get_gpu_elf_pool(L, &fatbin_buf, &filter, &pool);;
    if (status != 0) {
        return status;
    }

    lua_newtable(L);

    int table_index = lua_gettop(L);
    if (table_index < 0) table_index = lua_gettop(L) + table_index + 1;

    char *elf_basename = basename((char*) elf_filename);

    metadata_callback_ctx_t ctx = {
        .L = L,
        .table_index = table_index,
        .elf_filename = elf_basename
    };

    // Iterate ELF pool and fill Lua table
    liborkh_gpu_elf_pool_iterate(pool, set_metadata_buffer, &ctx);
    
    liborkh_gpu_elf_pool_free(pool);
    return 1;
}


/**
 * Free metadata buffer allocated by liborkh
 * Lua arguments:
 * 1. buffer (lightuserdata): Pointer to metadata buffer
 */
static int l_free_metadata_buffer(lua_State* L) {
    void *ptr = lua_touserdata(L, 1);  // get lightuserdata
    if (ptr == NULL)
        return luaL_error(L, "expected lightuserdata as argument");

    free(ptr);
    return 0;
}


/**
 * Lua module entry point
 */
int luaopen_liborkh_lua(lua_State *L) {
    luaL_Reg funcs[] = {
        {"get_kernel_count", l_get_kernel_count},
        {"get_metadata_buffer", l_get_metadata_buffer},
        {"free_metadata_buffer", l_free_metadata_buffer},
        {NULL, NULL}
    };

    luaL_register(L, "liborkh", funcs);
    return 1;
}
