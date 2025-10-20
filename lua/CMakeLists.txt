cmake_minimum_required(VERSION 3.16)

project(liborkh_lua C)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---- Dependencies ----
if (NOT DEFINED LUA_INCLUDE_DIR)
    message(FATAL_ERROR "LUA_INCLUDE_DIR must be provided to build the liborkh_lua module")
endif()

# ---- Source ----
set(SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/liborkh_lua.c
)

# ---- Build the Lua module ----
add_library(liborkh_lua MODULE ${SOURCE})

set_target_properties(liborkh_lua PROPERTIES
    PREFIX "" 
    OUTPUT_NAME "liborkh_lua"
)

# ---- Include dirs ----
target_include_directories(liborkh_lua PRIVATE
    ${LUA_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/include
)

# ---- Link dependencies ----
target_link_libraries(liborkh_lua
    PRIVATE
        liborkh
)

# ---- install ----
install(TARGETS liborkh_lua
    LIBRARY DESTINATION lib/lua
)