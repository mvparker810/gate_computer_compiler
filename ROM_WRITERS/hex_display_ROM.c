/*

Hex to ASCII ROM Generator

Generates three ROMs:
1. HEX_4_ASCII (256 entries): 4-bit input -> 8-bit ASCII output
   - Input: 4-bit value (0-15)
   - Output: ASCII hex digit ('0'-'9', 'A'-'F'), rest are 0x00

2. HEX_8_ASCII_LOWER (256 entries): 8-bit input -> 16-bit output
   - Output: (ascii_of_upper_nibble << 8) | ascii_of_lower_nibble (lowercase)

3. HEX_8_ASCII_UPPER (256 entries): 8-bit input -> 16-bit output
   - Output: (ascii_of_upper_nibble << 8) | ascii_of_lower_nibble (uppercase)

Example: For 0xAB (8-bit input):
- HEX_8_ASCII_LOWER[0xAB] -> 0x6262 ('b' + 'b')
- HEX_8_ASCII_UPPER[0xAB] -> 0x4141 ('A' + 'A')

Compilation (from gate_computer_compiler/ root):
$ gcc v2/text/hex/hex_rom.c v2/rom_writer.c -o out/hex_rom

Usage:
$ ./hex_rom [-f FORMAT]

Options:
- -f <FORMAT>: Output format: hex, uint, int, binary (default: hex)

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../utils/rom_writer.h"

static rom_format HEX_FORMAT = ROM_HEX;

// Convert a 4-bit nibble (0-15) to ASCII hex digit (uppercase: 0-9, A-F)
uint8_t nibble_to_ascii_upper(uint8_t nibble) {
    if (nibble < 10) {
        return '0' + nibble;      // 0-9 map to '0'-'9'
    } else {
        return 'A' + (nibble - 10); // 10-15 map to 'A'-'F' (uppercase)
    }
}

// Convert a 4-bit nibble (0-15) to ASCII hex digit (lowercase: 0-9, a-f)
uint8_t nibble_to_ascii_lower(uint8_t nibble) {
    if (nibble < 10) {
        return '0' + nibble;      // 0-9 map to '0'-'9'
    } else {
        return 'a' + (nibble - 10); // 10-15 map to 'a'-'f' (lowercase)
    }
}

int main(int argc, char* argv[]) {
    // Parse optional format flag
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -f requires an argument\n");
                return -1;
            }
            const char* format = argv[++i];
            if (strcmp(format, "hex") == 0) HEX_FORMAT = ROM_HEX;
            else if (strcmp(format, "uint") == 0) HEX_FORMAT = ROM_UINT;
            else if (strcmp(format, "int") == 0) HEX_FORMAT = ROM_INT;
            else if (strcmp(format, "binary") == 0) HEX_FORMAT = ROM_BINARY;
            else {
                fprintf(stderr, "Error: Unknown format '%s'\n", format);
                return -1;
            }
        }
    }

    // Allocate ROM data
    uint16_t hex_4_data[256] = {0};
    uint16_t hex_8_lower_data[256] = {0};
    uint16_t hex_8_upper_data[256] = {0};

    // 1. Generate HEX_4_ASCII: 256 entries, with actual data at 0-15 (uppercase), rest 0x00
    for (int i = 0; i < 256; i++) {
        if (i < 16) {
            hex_4_data[i] = nibble_to_ascii_upper(i);
        } else {
            hex_4_data[i] = 0x00;
        }
    }

    // 2. Generate HEX_8_ASCII_LOWER and HEX_8_ASCII_UPPER
    for (int addr = 0; addr < 256; addr++) {
        uint8_t lower_nibble = addr & 0xF;
        uint8_t upper_nibble = (addr >> 4) & 0xF;

        // Both nibbles as lowercase
        hex_8_lower_data[addr] = ((uint16_t)nibble_to_ascii_lower(upper_nibble) << 8) | nibble_to_ascii_lower(lower_nibble);

        // Both nibbles as uppercase
        hex_8_upper_data[addr] = ((uint16_t)nibble_to_ascii_upper(upper_nibble) << 8) | nibble_to_ascii_upper(lower_nibble);
    }

    // Write all three ROM files
    if (write_rom_file("v2/text/hex/out/HEX_4_ASCII.out", hex_4_data, 256, HEX_FORMAT) < 0) {
        fprintf(stderr, "Error: Failed to write HEX_4_ASCII.out\n");
        return -1;
    }

    if (write_rom_file("v2/text/hex/out/HEX_8_ASCII_LOWER.out", hex_8_lower_data, 256, HEX_FORMAT) < 0) {
        fprintf(stderr, "Error: Failed to write HEX_8_ASCII_LOWER.out\n");
        return -1;
    }

    if (write_rom_file("v2/text/hex/out/HEX_8_ASCII_UPPER.out", hex_8_upper_data, 256, HEX_FORMAT) < 0) {
        fprintf(stderr, "Error: Failed to write HEX_8_ASCII_UPPER.out\n");
        return -1;
    }

    printf("Generated three ROM files in v2/text/hex/out/:\n");
    printf("  HEX_4_ASCII.out (256 entries, data at 0-15): 4-bit input -> ASCII hex digit (UPPERCASE)\n");
    printf("  HEX_8_ASCII_LOWER.out (256 entries): 8-bit input -> 16-bit output (lowercase)\n");
    printf("  HEX_8_ASCII_UPPER.out (256 entries): 8-bit input -> 16-bit output (UPPERCASE)\n");
    return 0;
}
