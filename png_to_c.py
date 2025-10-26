#!/usr/bin/env python3

import sys
import os

if len(sys.argv) != 4:
    print("Usage: python3 png_to_c.py <png_file> <output_h_file> <asset_name>")
    sys.exit(1)

png_file = sys.argv[1]
h_file = sys.argv[2]
asset_name = sys.argv[3]

with open(png_file, 'rb') as f:
    data = f.read()

upper_name = asset_name.upper()
var_name = asset_name.lower()
len_name = f"{asset_name.lower()}_len"

with open(h_file, 'w') as f:
    f.write(f'#ifndef {upper_name}_H\n')
    f.write(f'#define {upper_name}_H\n\n')
    f.write(f'unsigned char {var_name}[] = {{\n')

    for i in range(len(data)):
        if i % 16 == 0:
            f.write('    ')
        f.write(f'0x{data[i]:02X}')
        if i < len(data) - 1:
            f.write(',')
        if i % 16 == 15:
            f.write('\n')
        else:
            f.write(' ')
    if len(data) % 16 != 0:
        f.write('\n')

    f.write(f'}};\nunsigned int {len_name} = sizeof({var_name});\n\n#endif\n')
