/*

Branch Condition ROM Generator

Generates a ROM that determines whether to branch based on NZCV flags and condition code.

Address format (8 bits):
- Bits 7-4: NZCV flags (N, Z, C, V in that order)
- Bits 3-0: Condition code (0-15)

Output:
- 0xFFFF if branch should be taken
- 0x0000 if branch should NOT be taken

Branch conditions:
0: B_UNCOND   - Always branch
1: B_EQ       - Branch if Z=1 (equal)
2: B_NE       - Branch if Z=0 (not equal)
3: B_LT       - Branch if N!=V (signed less than)
4: B_LE       - Branch if Z=1 OR N!=V (signed less than or equal)
5: B_GT       - Branch if Z=0 AND N==V (signed greater than)
6: B_GE       - Branch if N==V (signed greater than or equal)
7: B_CS       - Branch if C=1 (carry set / unsigned >=)
8: B_CC       - Branch if C=0 (carry clear / unsigned <)
9: B_MI       - Branch if N=1 (minus/negative)
10: B_PL      - Branch if N=0 (plus/positive or zero)
11: B_VS      - Branch if V=1 (overflow set)
12: B_VC      - Branch if V=0 (overflow clear)
13: B_HI      - Branch if C=1 AND Z=0 (unsigned higher)
14: B_LS      - Branch if C=0 OR Z=1 (unsigned lower or same)
15: UNUSED

Compilation (from gate_computer_compiler/ root):
$ gcc v2/branch_condition_rom.c v2/rom_writer.c -o out/branch_condition_rom

Usage:
$ ./out/branch_condition_rom [-f FORMAT]

Options:
- -f <FORMAT>: Output format: hex, uint, int, binary (default: hex)

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../utils/rom_writer.h"

static rom_format OUTPUT_FORMAT = ROM_HEX;

int main(int argc, char* argv[]) {
    // Parse optional format flag
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -f requires an argument\n");
                return -1;
            }
            const char* format = argv[++i];
            if (strcmp(format, "hex") == 0) OUTPUT_FORMAT = ROM_HEX;
            else if (strcmp(format, "uint") == 0) OUTPUT_FORMAT = ROM_UINT;
            else if (strcmp(format, "int") == 0) OUTPUT_FORMAT = ROM_INT;
            else if (strcmp(format, "binary") == 0) OUTPUT_FORMAT = ROM_BINARY;
            else {
                fprintf(stderr, "Error: Unknown format '%s'\n", format);
                return -1;
            }
        }
    }

    // Allocate ROM data (256 entries for all flag/condition combinations)
    uint16_t rom_data[256] = {0};

    // For each address (NZCV flags + condition code)
    for (int addr = 0; addr < 256; addr++) {
        // Extract NZCV flags from upper 4 bits
        uint8_t nzcv = (addr >> 4) & 0xF;
        uint8_t N = (nzcv >> 3) & 1;  // Bit 3
        uint8_t Z = (nzcv >> 2) & 1;  // Bit 2
        uint8_t C = (nzcv >> 1) & 1;  // Bit 1
        uint8_t V = nzcv & 1;         // Bit 0

        // Extract condition code from lower 4 bits
        uint8_t condition = addr & 0xF;

        // Evaluate branch condition
        int should_branch = 0;

        switch (condition) {
            case 0:  // B_UNCOND - Always branch
                should_branch = 1;
                break;
            case 1:  // B_EQ - Branch if Z=1
                should_branch = Z;
                break;
            case 2:  // B_NE - Branch if Z=0
                should_branch = !Z;
                break;
            case 3:  // B_LT - Branch if N != V (signed less than)
                should_branch = (N != V);
                break;
            case 4:  // B_LE - Branch if Z=1 OR N!=V (signed less than or equal)
                should_branch = Z || (N != V);
                break;
            case 5:  // B_GT - Branch if Z=0 AND N==V (signed greater than)
                should_branch = (!Z) && (N == V);
                break;
            case 6:  // B_GE - Branch if N==V (signed greater than or equal)
                should_branch = (N == V);
                break;
            case 7:  // B_CS - Branch if C=1 (carry set / unsigned >=)
                should_branch = C;
                break;
            case 8:  // B_CC - Branch if C=0 (carry clear / unsigned <)
                should_branch = !C;
                break;
            case 9:  // B_MI - Branch if N=1 (minus/negative)
                should_branch = N;
                break;
            case 10: // B_PL - Branch if N=0 (plus/positive or zero)
                should_branch = !N;
                break;
            case 11: // B_VS - Branch if V=1 (overflow set)
                should_branch = V;
                break;
            case 12: // B_VC - Branch if V=0 (overflow clear)
                should_branch = !V;
                break;
            case 13: // B_HI - Branch if C=1 AND Z=0 (unsigned higher)
                should_branch = C && (!Z);
                break;
            case 14: // B_LS - Branch if C=0 OR Z=1 (unsigned lower or same)
                should_branch = (!C) || Z;
                break;
            case 15: // UNUSED
            default:
                should_branch = 0;
                break;
        }

        // Output 0xFFFF if branch, 0x0000 if no branch
        rom_data[addr] = should_branch ? 0xFFFF : 0x0000;
    }

    // write ROM file
    if (write_rom_file("v2/out/BRANCH_CONDITION.out", rom_data, 256, OUTPUT_FORMAT) < 0) {
        fprintf(stderr, "Error: Failed to write ROM file\n");
        return -1;
    }

    printf("Generated branch condition ROM to v2/out/BRANCH_CONDITION.out\n");
    printf("Address = [NZCV flags (4 bits)][condition code (4 bits)]\n");
    printf("Value = 0xFFFF (branch) or 0x0000 (no branch)\n");
    return 0;
}
