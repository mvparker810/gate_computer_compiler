/*

Compilation (from gate_computer_compiler/ root):
$ gcc v2/machine_code_translator_rom.c v2/rom_writer.c -o out/machine_code_translator_rom

Usage:
$ ./out/machine_code_translator_rom [-f FORMAT]

Options:
--f <FORMAT>: Output format: hex, uint, int, binary (default: hex)


gcc v2/fp16_digit_masks.c ROM_WRITERS/rom_writer.c -o out/fp16_digit_masks
/out/machine_code_translator_rom -f hex

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

    uint16_t rom_data[256] = {0};

    typedef enum {
        FPC_ZERO = 0x8,
        FPC_NUM  = 0x4,
        FPC_INF  = 0x2,
        FPC_NAN  = 0x1,
    } FPCODE;

    static const FPCODE fp_codes[] = {
        FPC_ZERO,
        FPC_NUM,
        FPC_INF,
        FPC_NAN
    };

    for (int i = 0; i < sizeof(fp_codes)/sizeof(fp_codes[0]); i++) {
        FPCODE code = fp_codes[i];  // upper nibble enum

        for (uint8_t cell = 0; cell <= 9; cell++) {   // only real digits 0–9
            
            // Encode address: [fp_code:cell_idx]
            uint8_t addr = ((uint8_t)code << 4) | cell;
            uint8_t character = 0;
            //1 is 
            switch (code) {
                case FPC_ZERO:
                    character = "         0"[cell];
                break;
                case FPC_NUM:
                    character = 0;
                break;
                case FPC_INF:
                    character = "       Inf"[cell];
                break;
                case FPC_NAN:
                    character = "       NaN"[cell];
              
                break;
            }

            rom_data[addr] = (character << 8) | character;
        }
    }



    if (write_rom_file("rom_out/fp16_digitmask", rom_data, 256, HEX_FORMAT) < 0) {
        fprintf(stderr, "Error: Failed to write HEX_4_ASCII.out\n");
        return -1;
    }

    return 0;
}
