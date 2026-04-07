cmake_minimum_required(VERSION 3.10)

set(LIBORKH_LUA liborkh_lua)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---- Dependencies ----
if (NOT DEFINED LUA_INCLUDE_DIR)
    message(FATAL_ERROR "LUA_INCLUDE_DIR must be provided to build the ${LIBORKH_LUA} module")
endif()

# ---- Source ----
set(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/liborkh_lua.c)

# ---- Build the Lua module ----
add_library(${LIBORKH_LUA} MODULE ${SOURCE})

set_target_properties(${LIBORKH_LUA} PROPERTIES
    PREFIX "" 
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/lua 
    OUTPUT_NAME "${LIBORKH_LUA}"
)

# ---- Include dirs ----
target_include_directories(${LIBORKH_LUA} PRIVATE
    ${LUA_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/include
)

# ---- Link dependencies ----
target_link_libraries(${LIBORKH_LUA} PRIVATE orkh)