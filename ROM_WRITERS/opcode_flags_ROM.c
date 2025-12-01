/*

Machine Code Translator ROM Generator

Generates a ROM that describes properties of each opcode for instruction decoding.
Address = Opcode (0x00-0xFF)
Value = Flags describing instruction properties

Flags (16-bit value):
- Bit 0: Valid instruction
- Bits 1-4: Instruction type (0=ALU, 1=MOVE, 2=CMP, 3=BRANCH, 4=MEMORY, 5=PRINT_REG, 6=PRINT_CONST)
- Bit 5: Is immediate format variant
- Bit 11: OVERRIDE WRITE flag (for MOV commands)
- Bit 12: OVERRIDE B flag (for ALU_I commands)
- Bit 13: Try read A operand
- Bit 14: Try read B operand
- Bit 15: Try write result

Compilation (from gate_computer_compiler/ root):
$ gcc v2/machine_code_translator_rom.c v2/rom_writer.c -o out/machine_code_translator_rom

Usage:
$ ./out/machine_code_translator_rom [-f FORMAT]

Options:
- -f <FORMAT>: Output format: hex, uint, int, binary (default: hex)

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../utils/rom_writer.h"

static rom_format OUTPUT_FORMAT = ROM_HEX;

// Flag bit definitions
#define FLAG_VALID          (1 << 0)  // Bit 0: Valid instruction
#define FLAG_TYPE_ALU       (0 << 1)  // Bits 1-4: Instruction type
#define FLAG_TYPE_MOVE      (1 << 1)
#define FLAG_TYPE_CMP       (2 << 1)
#define FLAG_TYPE_BRANCH    (3 << 1)
#define FLAG_TYPE_MEMORY    (4 << 1)
#define FLAG_TYPE_PRINT_REG (5 << 1)  // PRINT register data (position determined by IMMEDIATE flag)
#define FLAG_TYPE_PRINT_CONST (6 << 1) // PRINT constant data (position determined by IMMEDIATE flag)
#define FLAG_TYPE_MASK      (15 << 1)
#define FLAG_IMMEDIATE      (1 << 5)  // Bit 5: Is immediate format
#define FLAG_OVERRIDE_WRITE (1 << 11) // Bit 11: OVERRIDE WRITE flag (for MOV commands)
#define FLAG_OVERRIDE_B     (1 << 12) // Bit 12: OVERRIDE B flag (for ALU_I commands)
#define FLAG_TRY_READ_A     (1 << 13) // Bit 13: Try read A operand
#define FLAG_TRY_READ_B     (1 << 14) // Bit 14: Try read B operand
#define FLAG_TRY_WRITE      (1 << 15) // Bit 15: Try write result

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

    // Allocate ROM data (256 entries for all opcodes)
    uint16_t rom_data[256] = {0};

    // ========== TYPE 1: ALU Operations ==========

    // 0x00-0x09: ALU Register Format (R format)
    for (int i = 0x00; i <= 0x09; i++) {
        rom_data[i] = FLAG_VALID | FLAG_TYPE_ALU |
                      FLAG_TRY_READ_A | FLAG_TRY_READ_B | FLAG_TRY_WRITE;
    }

    // 0x10-0x19: ALU Immediate Format (I format)
    for (int i = 0x10; i <= 0x19; i++) {
        rom_data[i] = FLAG_VALID | FLAG_TYPE_ALU | FLAG_IMMEDIATE |
                      FLAG_OVERRIDE_B | FLAG_TRY_READ_A | FLAG_TRY_WRITE;
    }

    // ========== TYPE 2: MOVE Operations ==========

    // 0x20: MOV (Register format - R[DST] = R[SRC])
    rom_data[0x20] = FLAG_VALID | FLAG_TYPE_MOVE |
                     FLAG_TRY_READ_A | FLAG_TRY_READ_B | FLAG_TRY_WRITE;

    // 0x21: MOV_I (Immediate format - R[DST] = IMMEDIATE)
    rom_data[0x21] = FLAG_VALID | FLAG_TYPE_MOVE | FLAG_IMMEDIATE | FLAG_OVERRIDE_WRITE |
                     FLAG_TRY_READ_A | FLAG_TRY_WRITE;

    // ========== TYPE 2: CMP Operations ==========

    // 0x22: CMP (Register format - FLAGS = R[A] ~ R[B])
    rom_data[0x22] = FLAG_VALID | FLAG_TYPE_CMP |
                     FLAG_TRY_READ_A | FLAG_TRY_READ_B;

    // 0x23: CMP_I (Immediate format - FLAGS = R[A] ~ IMMEDIATE)
    rom_data[0x23] = FLAG_VALID | FLAG_TYPE_CMP | FLAG_IMMEDIATE | FLAG_OVERRIDE_WRITE |
                     FLAG_TRY_READ_A;

    // ========== TYPE 3: BRANCH Operations ==========

    // 0x24: B (Conditional Branch with condition field)
    rom_data[0x24] = FLAG_VALID | FLAG_TYPE_BRANCH | FLAG_TRY_READ_B;

    // 0x25: B_I (Conditional Branch with immediate address)
    rom_data[0x25] = FLAG_VALID | FLAG_TYPE_BRANCH | FLAG_IMMEDIATE | FLAG_OVERRIDE_B;

    // ========== TYPE 4: MEMORY Operations ==========

    // 0x26: READ (Register Addressing - R[DST] = MEM[R[A]])
    rom_data[0x26] = FLAG_VALID | FLAG_TYPE_MEMORY |
                     FLAG_TRY_READ_B | FLAG_TRY_WRITE;

    // 0x27: READ_I (Immediate Addressing - R[DST] = MEM[IMMEDIATE[3:0]])
    rom_data[0x27] = FLAG_VALID | FLAG_TYPE_MEMORY | FLAG_IMMEDIATE |
                     FLAG_TRY_WRITE | FLAG_OVERRIDE_B;

    // 0x28: WRITE (Register Addressing - MEM[R[B]] = R[A])
    rom_data[0x28] = FLAG_VALID | FLAG_TYPE_MEMORY |
                     FLAG_TRY_READ_A | FLAG_TRY_READ_B;

    // 0x29: WRITE_I (Immediate Addressing - MEM[IMMEDIATE[3:0]] = R[A])
    rom_data[0x29] = FLAG_VALID | FLAG_TYPE_MEMORY | FLAG_IMMEDIATE |
                     FLAG_TRY_READ_A | FLAG_OVERRIDE_B;

    // ========== TYPE 5-6: PRINT Operations ==========

    // 0x2A: PRINT_REG (Register position - SCN[R[B]] = R[A])
    rom_data[0x2A] = FLAG_VALID | FLAG_TYPE_PRINT_REG |
                     FLAG_TRY_READ_A | FLAG_TRY_READ_B;

    // 0x2B: PRINT_REG_I (Immediate position - SCN[H] = R[A])
    rom_data[0x2B] = FLAG_VALID | FLAG_TYPE_PRINT_REG | FLAG_IMMEDIATE |
                     FLAG_TRY_READ_A | FLAG_OVERRIDE_WRITE;

    // 0x2C: PRINT_CONST (Register position - SCN[R[B]] = K)
    rom_data[0x2C] = FLAG_VALID | FLAG_TYPE_PRINT_CONST |
                     FLAG_TRY_READ_B | FLAG_OVERRIDE_WRITE;

    // 0x2D: PRINT_CONST_I (Immediate position - SCN[H] = K)
    rom_data[0x2D] = FLAG_VALID | FLAG_TYPE_PRINT_CONST | FLAG_IMMEDIATE | FLAG_OVERRIDE_WRITE;

    // ========== Legacy MEMORY Operations ==========

    // 0x09: MEMORY (Constant Addressing - 8-bit address)
    rom_data[0x09] = FLAG_VALID | FLAG_TYPE_MEMORY |
                     FLAG_TRY_READ_A | FLAG_TRY_READ_B | FLAG_TRY_WRITE;

    // 0x0C: MEMI (Register Addressing)
    rom_data[0x0C] = FLAG_VALID | FLAG_TYPE_MEMORY |
                     FLAG_TRY_READ_A | FLAG_TRY_READ_B | FLAG_TRY_WRITE;

    // Write ROM file
    if (write_rom_file("v2/out/MACHINE_CODE_TRANSLATOR.out", rom_data, 256, OUTPUT_FORMAT) < 0) {
        fprintf(stderr, "Error: Failed to write ROM file\n");
        return -1;
    }

    printf("Generated machine code translator ROM to v2/out/MACHINE_CODE_TRANSLATOR.out\n");
    printf("Address = Opcode (0x00-0xFF)\n");
    printf("Value = Instruction property flags\n");
    return 0;
}
