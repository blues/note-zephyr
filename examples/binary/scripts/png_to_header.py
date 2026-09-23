#!/usr/bin/env python3
"""Convert a PNG file to a C header containing a byte array.

Usage: png_to_header.py <input.png> <output.h> <variable_name>
"""

import sys
import os


def png_to_header(png_path, output_path, var_name):
    with open(png_path, 'rb') as f:
        data = f.read()

    rows = [data[i:i+12] for i in range(0, len(data), 12)]
    lines = ['    ' + ', '.join(f'0x{b:02X}' for b in row) + ',' for row in rows]

    guard = os.path.basename(output_path).upper().replace('.', '_').replace('-', '_')

    header = f"""\
/*
 * Auto-generated from {os.path.basename(png_path)} — do not edit manually.
 */

#ifndef {guard}
#define {guard}

#include <stdint.h>

static const uint8_t {var_name}[] = {{
{chr(10).join(lines)}
}};

static const uint32_t {var_name}_len = {len(data)};

#endif /* {guard} */
"""

    with open(output_path, 'w') as f:
        f.write(header)


if __name__ == '__main__':
    if len(sys.argv) != 4:
        print(f'Usage: {sys.argv[0]} <input.png> <output.h> <variable_name>')
        sys.exit(1)
    png_to_header(sys.argv[1], sys.argv[2], sys.argv[3])
