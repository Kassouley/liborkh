# liborkh

`liborkh` is a small C library for extracting GPU offload binaries (ELF) 
from LLVM/Clang offload-packaged or bundled ELF files. 
It provides functions to open ELF files, extract GPU sections, decode GPU packages/bundles binaries, 
and save them to separate files.

## Features

- Open ELF files and extract GPU fatbin sections.
- Decode packager/bundle format.
- Extract GPU ELF images and metadata (target triple, architecture, image type, offload kind).
- Save extracted GPU binaries to filesystem with structured filenames.
- Retrieve kernels metadata as a buffer and number of kernels in ELF.

## Packager/Bundler format

More information :
- Clang Offload Packager : https://rocm.docs.amd.com/projects/llvm-project/en/latest/LLVM/clang/html/ClangOffloadPackager.html
- Clang Offload Bundler : https://rocm.docs.amd.com/projects/llvm-project/en/latest/LLVM/clang/html/ClangOffloadBundler.html


## Build Instructions

Requires **CMake >= 3.16** and **libelf**.

```bash
# Build the library and test binaries
mkdir build && cd build
cmake ..
make
```

This will generate:

- liborkh.so (shared library)
- extract_gpu_elf (test executable)
- extract_gpu_fatbin (test executable)
- print_nb_kernels (test executable)

## Usage

### Command-Line

Two test executables are provided to extract GPU binaries from ELF files:

```bash
./extract_gpu_elf <input-elf-file>
./extract_gpu_fatbin <input-elf-file> <output>
./print_nb_kernels <input-elf-file>
```

Input :
- <input-elf-file> - the ELF file containing GPU ELF.
- <output> - the output file to write the gpu fatbin.

GPU ELF files follow the naming convention:
```bash
<input_filename>:<id>.<image>.<offload_kind>.<triple>.<arch>
```

### Programmatic API

You can use `liborkh` directly in C programs:

```c
#include <stdio.h>
#include <stdlib.h>
#include "liborkh.h"

int main(int argc, char **argv) {
    const char *elf_filename = "./a.out";
    const char *output       = "output.fatbin";

    Elf *elf = NULL;
    if (liborkh_open_elf(elf_filename, &elf) != 0) {
        return 1;
    }
    
    liborkh_offload_buffer fatbin_buf = {0};
    if (liborkh_extract_gpu_fatbin(elf, &fatbin_buf) != 0) {
        liborkh_close_elf(elf);
        return 1;
    }

    liborkh_write_fatbin_to_file(&fatbin_buf, output);

    liborkh_close_elf(elf);
    return 0;
}
```