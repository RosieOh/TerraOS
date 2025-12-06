#!/usr/bin/env python3
# tools/elf_to_c.py - ELF 바이너리를 C 배열로 변환

import sys

def elf_to_c_array(elf_path, var_name):
    with open(elf_path, 'rb') as f:
        data = f.read()
    
    print(f"// Auto-generated from {elf_path}")
    print(f"static const unsigned char {var_name}[] = {{")
    
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        hex_str = ', '.join(f'0x{b:02x}' for b in chunk)
        print(f"    {hex_str},")
    
    print(f"}};")
    print(f"static const uint32_t {var_name}_size = {len(data)};")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: elf_to_c.py <elf_file> <var_name>")
        sys.exit(1)
    
    elf_to_c_array(sys.argv[1], sys.argv[2])

