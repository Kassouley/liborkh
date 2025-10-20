package.cpath =  "../build/lua/?.so;" .. package.cpath
local liborkh = require("liborkh_lua")

local filter = {
    target_arch = "gfx1030"
}

local elf_file = arg[1]
if not elf_file then
    print("Usage: lua test_liborkh.lua <path_to_elf_file>")
    os.exit(1)
end

local count = liborkh.get_kernel_count(elf_file, filter)
print("Number of kernels in ELF:", count)

local results = liborkh.get_metadata_buffer(elf_file)

for name, entry in pairs(results) do
    print("ELF:", name)
    print("  Metadata pointer:", entry.data)
    print("  Metadata size:", entry.size)

    liborkh.free_metadata_buffer(entry.data)
end

print("All metadata processed and freed")
