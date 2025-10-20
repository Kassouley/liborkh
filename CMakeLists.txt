cmake_minimum_required(VERSION 3.16)
project(liborkh C)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---- Include directories ----
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# ---- Source files for the shared library ----
FILE(GLOB_RECURSE LIBORKH_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# ---- Create shared library ----
add_library(liborkh SHARED ${LIBORKH_SRC})

# Link with libelf
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBELF REQUIRED libelf)

target_include_directories(liborkh PUBLIC  ${LIBELF_INCLUDE_DIRS})
target_link_libraries     (liborkh         ${LIBELF_LIBRARIES})
target_compile_options    (liborkh PRIVATE ${LIBELF_CFLAGS_OTHER})

# ---- Lua module ----
add_subdirectory(lua)

# ---- Test binaries ----
function(add_liborkh_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE liborkh)
    target_include_directories(${name} PRIVATE include)
endfunction()


add_liborkh_test(extract_gpu_elf        tests/main_extract_gpu_elf.c)
add_liborkh_test(extract_gpu_fatbin     tests/main_extract_gpu_fatbin.c)
add_liborkh_test(print_nb_kernel        tests/main_print_nb_kernel.c)
add_liborkh_test(print_total_nb_kernels tests/main_print_total_nb_kernels.c)

