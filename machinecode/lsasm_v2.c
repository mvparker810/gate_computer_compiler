/*

LSASM V2 - 32-bit ISA Assembler

Outputs two 256×16 ROM files (ALPHA and BETA) that combine to form 32-bit instructions.
- ALPHA ROM: Upper 16 bits (address = instruction address)
- BETA ROM: Lower 16 bits (address = instruction address)

Compilation (from gate_computer_compiler/ root):

$ gcc v2/computer/lsasm_v2.c v2/rom_writer.c -o out/lsasm_v2

Usage:
$ ./out/lsasm_v2 [-f FORMAT] <input_file> <base_name>

Examples:
$ ./out/lsasm_v2 v2/scripts/fib.txt fib
$ ./out/lsasm_v2 -f binary v2/scripts/fib.txt program

This generates:
- fib_ALPHA.out (or fib_ALPHA with appropriate extension)
- fib_BETA.out (or fib_BETA with appropriate extension)

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include "../utils/rom_writer.h"

#define MAX_INSTR 256
#define MAX_LINE 256


typedef enum {
    ALU_AND = 0x00,
    ALU_OR = 0x01,
    ALU_XOR = 0x02,
    ALU_NOT = 0x03,
    ALU_ADD = 0x04,
    ALU_SUB = 0x05,
    ALU_LSL = 0x06,
    ALU_LSR = 0x07,
    ALU_BCD_LOW = 0x08,
    ALU_BCD_HIGH = 0x09,
    //ALU imm is the same but 0x1N


    MOVE = 0x20,
    MOVE_I = 0x21,

    CMP = 0x22,
    CMP_I = 0x23,
    B = 0x24,
    B_I = 0x25,

    READ = 0x26,
    READ_I = 0x27,
    WRITE = 0x28,
    WRITE_I = 0x29,

    PRINT_REG = 0x2A,
    PRINT_REG_I = 0x2B,
    PRINT_CONST = 0x2C,
    PRINT_CONST_I = 0x2D,

    EXIT_OP = 0xFFFF
} opcode;

typedef enum {
    B_UNCOND = 0,
    B_EQ = 1,       // BEQ (Z = 1)
    B_NE = 2,       // BNE (Z = 0)
    B_LT = 3,       // BLT (N != V, signed LT)
    B_LE = 4,       // BLE (Z = 1 OR N != V, signed LEQ)
    B_GT = 5,       // BGT (Z = 0 AND N == V, signed GT)
    B_GE = 6,       // BGE (N == V, signed GEQ)
    B_CS = 7,       // BCS (C = 1, carry set   / unsigned >=)
    B_CC = 8,       // BCC (C = 0, carry clear / unsigned <)
    B_MI = 9,       // BMI (N = 1, minus/negative)
    B_PL = 10,      // BPL (N = 0, plus/positive or zero)
    B_VS = 11,      // BVS (V = 1, overflow set)
    B_VC = 12,      // BVC (V = 0, overflow clear)
    B_HI = 13,      // BHI (C = 1 AND Z = 0, unsigned higher)
    B_LS = 14       // BLS (C = 0 OR Z = 1, unsigned  lower or same)
} branch_condition;

#define MAX_LABELS 256
typedef struct {
    char name[32];
    uint8_t address;
} Label;
typedef struct {
    Label labels[MAX_LABELS];
    int count;
} SymbolTable;







void symbol_table_init(SymbolTable* table) {
    table->count = 0;
}

int symbol_table_add(SymbolTable* table, const char* name, uint8_t address) {
    if (table->count >= MAX_LABELS) return -1;
    strncpy(table->labels[table->count].name, name, 31);
    table->labels[table->count].name[31] = '\0';
    table->labels[table->count].address = address;
    table->count++;
    return 0;
}

int symbol_table_lookup(SymbolTable* table, const char* name) {
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->labels[i].name, name) == 0) {
            return table->labels[i].address;
        }
    }
    return -1; // not found
}

// labels cannot be instructions and must have alphanumeric or underscore chars
int is_label(const char* line) {
    while (*line && isspace(*line)) line++;
    if (!*line) return 0;
    const char* colon = strchr(line, ':');
    if (!colon) return 0;

    char temp[32];
    int len = colon - line;
    if (len <= 0 || len >= 32) return 0;
    strncpy(temp, line, len);
    temp[len] = '\0';

    if (!isalpha(temp[0]) && temp[0] != '_') return 0;
    return 1;
}

//get label name
void parse_label(const char* line, char* label) {
    while (*line && isspace(*line)) line++;
    int i = 0;
    while (*line && *line != ':' && i < 31) {
        label[i++] = *line++;
    }
    label[i] = '\0';
}


// Helper function to parse register (e.g., "X0" -> 0)
uint8_t parse_register(const char* str) {
    if (str[0] != 'X' && str[0] != 'x') return -1;
    int reg = atoi(str + 1);
    if (reg < 0 || reg > 7) return -1;
    return (uint8_t)reg;
}

// Helper function to parse constant (hex, binary, decimal, or ASCII) - supports 16-bit values
uint16_t parse_constant(const char* str) {
    int value;

    // Check for ASCII character literal: 'A' -> 65
    if (str[0] == '\'' && str[2] == '\'' && str[3] == '\0') {
        return (uint16_t)(unsigned char)str[1];
    }

    if (str[0] == '0' && str[1]) {
        if (str[1] == 'x' || str[1] == 'X') {
            // Hexadecimal: 0xFF or 0xFFFF
            value = (int)strtol(str, NULL, 16);
        } else if (str[1] == 'b' || str[1] == 'B') {
            // Binary: 0b11110000
            value = (int)strtol(str + 2, NULL, 2);
        } else {
            // Decimal: 255
            value = atoi(str);
        }
    } else {
        // Decimal: 255
        value = atoi(str);
    }
    if (value < 0 || value > 65535) return -1;
    return (uint16_t)value;
}

// Strip C-style comments (// and /* */) from line
// inMultiline tracks if we're inside a /* */ comment across lines
void strip_comments(char* line, int* inMultiline) {
    int i = 0, j = 0;
    int len = strlen(line);

    while (i < len) {
        // Check if we're in a multiline comment
        if (*inMultiline) {
            // Look for end of multiline comment */
            if (i < len - 1 && line[i] == '*' && line[i+1] == '/') {
                *inMultiline = 0;
                i += 2;
            } else {
                i++;
            }
        } else {
            // Check for start of multiline comment /*
            if (i < len - 1 && line[i] == '/' && line[i+1] == '*') {
                *inMultiline = 1;
                i += 2;
            }
            // Check for single-line comment //
            else if (i < len - 1 && line[i] == '/' && line[i+1] == '/') {
                // Rest of line is comment, truncate here
                line[j] = '\0';
                return;
            }
            // Regular character
            else {
                line[j++] = line[i++];
            }
        }
    }

    line[j] = '\0';
}


uint32_t encode_alu(opcode op, uint8_t dst, uint16_t src1, uint16_t src2, uint8_t altBit) {
    uint32_t instr = 0;
    // For immediate format, add 0x10 to the opcode (bit 4 is the immediate flag)
    uint8_t actual_opcode = altBit ? (op | 0x10) : op;
    instr |= actual_opcode;              // [0-7] : opcode (includes immediate flag in bit 4)
    instr |= ((dst & 0x7) << 8);         // [8-10] : destination register

    if (!altBit) {
        // REG ~ REG mode   : SRC1 [12-14], src2 [16-18]
        instr |= ((src1 & 0x7) << 12);   // [12-14] : A register
        instr |= ((src2 & 0x7) << 16);   // [16-18] : B register
    } else {
        // REG ~ CONST mode : A register [12-14], CONST [16-31]
        instr |= ((src1 & 0x7) << 12);   // [12-14] : A register
        instr |= ((src2 & 0xFFFF) << 16);  // [16-31] : 16-bit immediate constant
    }
    return instr;
}

uint32_t encode_move(uint8_t dst, uint16_t src_or_imm, uint8_t altBit) {
    uint32_t instr = 0;
    // Use MOVE (0x20) for register mode, MOVE_I (0x21) for immediate mode
    if (altBit) {
        instr |= MOVE_I;  // [0-7] : opcode 0x21 (immediate)
    } else {
        instr |= MOVE;    // [0-7] : opcode 0x20 (register)
    }
    instr |= ((dst & 0x7) << 8);   // [8-10] : destination register

    // REG = REG mode  : src in bits 12-14 (A field)
    if (!altBit) {
        instr |= ((src_or_imm & 0x7) << 12);  // [12-14] : source register (A field)
    }
    // REG = CONST mode: immediate in bits 16-31
    else {
        instr |= ((uint32_t)(src_or_imm & 0xFFFF) << 16);  // [16-31] : 16-bit immediate
    }

    return instr;
}

uint32_t encode_cmp(uint8_t src1, uint16_t src2, uint8_t altBit) {
    uint32_t instr = 0;
    // For immediate format use opcode 0x23 (CMP_I), for register use 0x22 (CMP)
    uint8_t actual_opcode = altBit ? CMP_I : CMP;
    instr |= actual_opcode;             // [0-7] : opcode

    if (!altBit) {
        // REG ~ REG mode: src1 [12-14], src2 [16-18]
        instr |= ((src1 & 0x7) << 12);  // [12-14] : A register
        instr |= ((src2 & 0x7) << 16);  // [16-18] : B register
    } else {
        // REG ~ CONST mode: immediate [16-31]
        instr |= ((src1 & 0x7) << 12);  // [12-14] : A register
        instr |= ((uint32_t)(src2 & 0xFFFF) << 16);  // [16-31] : 16-bit immediate
    }
    return instr;
}

// Parse branch condition from mnemonic suffix (e.g., "BLE" -> B_LE)
int parse_branch_condition(const char* mnemonic) {
    if (strcmp(mnemonic, "B") == 0) return B_UNCOND;
    if (strcmp(mnemonic, "BEQ") == 0) return B_EQ;
    if (strcmp(mnemonic, "BNE") == 0) return B_NE;
    if (strcmp(mnemonic, "BLT") == 0) return B_LT;
    if (strcmp(mnemonic, "BLE") == 0) return B_LE;
    if (strcmp(mnemonic, "BGT") == 0) return B_GT;
    if (strcmp(mnemonic, "BGE") == 0) return B_GE;
    if (strcmp(mnemonic, "BCS") == 0) return B_CS;
    if (strcmp(mnemonic, "BCC") == 0) return B_CC;
    if (strcmp(mnemonic, "BMI") == 0) return B_MI;
    if (strcmp(mnemonic, "BPL") == 0) return B_PL;
    if (strcmp(mnemonic, "BVS") == 0) return B_VS;
    if (strcmp(mnemonic, "BVC") == 0) return B_VC;
    if (strcmp(mnemonic, "BHI") == 0) return B_HI;
    if (strcmp(mnemonic, "BLS") == 0) return B_LS;
    return -1;  // Invalid condition
}

uint32_t encode_branch(uint8_t condition, uint16_t target, uint8_t is_immediate) {
    uint32_t instr = 0;
    if (is_immediate) {
        // B_I (0x25): PC = IMMEDIATE
        // Format: OPCODE[8] Condition[4] [IMMEDIATE]
        instr |= B_I;  // [0-7] : opcode
        instr |= ((condition & 0xF) << 8);  // [8-11] : condition field
        instr |= ((uint32_t)(target & 0xFFFF) << 16);  // [16-31] : 16-bit immediate
    } else {
        // B (0x24): PC = R[A] (register in A field)
        // Format: OPCODE[8] Condition[4] [0000] A[3]0 [unused]
        instr |= B;  // [0-7] : opcode
        instr |= ((condition & 0xF) << 8);  // [8-11] : condition field
        // Bits 12-15 are zeros (already 0)
        instr |= ((target & 0x7) << 16);  // [16-18] : A register (target register number)
        // Bit 19 is 0 (already 0)
    }
    return instr;
}


// READ (0x26): R[DST] = MEM[R[B]] - Register format
uint32_t encode_read(uint8_t dst, uint8_t addr_reg) {
    uint32_t instr = 0;
    instr |= READ;                        // [0-7] : opcode (0x26)
    instr |= ((dst & 0x7) << 8);          // [8-10] : destination register
    instr |= ((addr_reg & 0x7) << 16);    // [16-18] : address register (B field)
    return instr;
}

// READ_I (0x27): R[DST] = MEM[IMMEDIATE[3:0]] - Immediate format
uint32_t encode_read_i(uint8_t dst, uint8_t addr_imm) {
    uint32_t instr = 0;
    instr |= READ_I;                      // [0-7] : opcode (0x27)
    instr |= ((dst & 0x7) << 8);          // [8-10] : destination register
    instr |= ((uint32_t)(addr_imm & 0xF) << 16);  // [16-19] : immediate address (lowest 4 bits)
    return instr;
}

// WRITE (0x28): MEM[R[B]] = R[A] - Register format
uint32_t encode_write(uint8_t data_reg, uint8_t addr_reg) {
    uint32_t instr = 0;
    instr |= WRITE;                       // [0-7] : opcode (0x28)
    instr |= ((data_reg & 0x7) << 12);    // [12-14] : data register (A field)
    instr |= ((addr_reg & 0x7) << 16);    // [16-18] : address register (B field)
    return instr;
}

// WRITE_I (0x29): MEM[IMMEDIATE[3:0]] = R[A] - Immediate format
uint32_t encode_write_i(uint8_t data_reg, uint8_t addr_imm) {
    uint32_t instr = 0;
    instr |= WRITE_I;                     // [0-7] : opcode (0x29)
    instr |= ((data_reg & 0x7) << 12);    // [12-14] : data register (A field)
    instr |= ((uint32_t)(addr_imm & 0xF) << 16);  // [16-19] : immediate address (lowest 4 bits)
    return instr;
}

// PRINT_REG (0x2A): SCN[R[B]] = R[A] - Register format
uint32_t encode_print_reg(uint8_t data_reg, uint8_t pos_reg) {
    uint32_t instr = 0;
    instr |= PRINT_REG;                   // [0-7] : opcode (0x2A)
    instr |= ((data_reg & 0x7) << 12);    // [12-14] : data register (A field)
    instr |= ((pos_reg & 0x7) << 16);     // [16-18] : position register (B field)
    return instr;
}

// PRINT_REG_I (0x2B): SCN[Y] = R[A] - Immediate format
// Y (bits [24-31]) = address, R[A] = code value
uint32_t encode_print_reg_i(uint8_t data_reg, uint8_t pos_imm) {
    uint32_t instr = 0;
    instr |= PRINT_REG_I;                 // [0-7] : opcode (0x2B)
    instr |= ((data_reg & 0x7) << 12);    // [12-14] : data register (A field)
    instr |= ((uint32_t)(pos_imm & 0xFF) << 24);  // [24-31] : address (high byte Y of IMMEDIATE)
    return instr;
}

// PRINT_CONST (0x2C): SCN[R[B]] = K - Register format
uint32_t encode_print_const(uint8_t data_const, uint8_t pos_reg) {
    uint32_t instr = 0;
    instr |= PRINT_CONST;                 // [0-7] : opcode (0x2C)
    instr |= ((data_const & 0x7) << 12);  // [12-14] : constant data (A field, 0-7)
    instr |= ((pos_reg & 0x7) << 16);     // [16-18] : position register (B field)
    return instr;
}

// PRINT_CONST_I (0x2D): SCN[Y] = X - Immediate format
// Y (bits [24-31]) = address, X (bits [16-23]) = code value (constant)
uint32_t encode_print_const_i(uint16_t data_const, uint8_t pos_imm) {
    uint32_t instr = 0;
    instr |= PRINT_CONST_I;               // [0-7] : opcode (0x2D)
    instr |= ((data_const & 0xFF) << 16);     // [16-23] : code value (low byte X of IMMEDIATE)
    instr |= ((uint32_t)(pos_imm & 0xFF) << 24);  // [24-31] : address (high byte Y of IMMEDIATE)
    return instr;
}

uint16_t encode_b(uint8_t condition, uint8_t target) {
    uint16_t instr = 0;
    instr |= B;                          // [0,3] : opcode (10)
    instr |= ((condition & 0xF) << 4);   // [4,7] : branch condition
    instr |= ((target & 0xFF) << 8);     // [8,15]: target address
    return instr;
}


// Helper function to parse and encode ALU instructions
uint32_t parse_alu_instruction(opcode op, char* operands, int* error) {
    uint8_t dst;
    uint16_t src1, src2;
    char dst_str[16], src1_str[16], src2_str[16];

    // Handle commas in operands (e.g., "X0, X0, 1" -> "X0 X0 1")
    char temp_operands[256];
    strncpy(temp_operands, operands, sizeof(temp_operands) - 1);
    temp_operands[sizeof(temp_operands) - 1] = '\0';
    for (int i = 0; temp_operands[i]; i++) {
        if (temp_operands[i] == ',') temp_operands[i] = ' ';
    }

    // Try 3 operands: dst src1 src2
    if (sscanf(temp_operands, "%s %s %s", dst_str, src1_str, src2_str) == 3) {
        dst = parse_register(dst_str);
        src1 = parse_register(src1_str);

        // Check if src2 is a register or constant
        if (src2_str[0] == 'X' || src2_str[0] == 'x') {
            // Both registers: use register mode (altBit=0)
            src2 = parse_register(src2_str);
            return encode_alu(op, dst, src1, src2, 0);
        } else {
            // src2 is a constant: use immediate mode (altBit=1)
            src2 = parse_constant(src2_str);
            return encode_alu(op, dst, src1, src2, 1);
        }
    }

    // Try 2 operands: dst src1
    if (sscanf(temp_operands, "%s %s", dst_str, src1_str) == 2) {
        dst = parse_register(dst_str);
        if (src1_str[0] == 'X' || src1_str[0] == 'x') {
            src2 = parse_register(src1_str);
            return encode_alu(op, dst, dst, src2, 0);  // REG OP REG (self as src1)
        } else {
            src1 = parse_constant(src1_str);
            return encode_alu(op, dst, src1, 0, 1);    // REG OP CONST
        }
    }

    *error = 1;
    return 0;
}

// Parse and encode a single instruction line
uint32_t parse_instruction(char* line, SymbolTable* symTable, int* error) {
    *error = 0;

    while (*line && isspace(*line)) line++;
    if (!*line || *line == ';' || *line == '#') return 0;
    if (is_label(line)) return 0;
    
    char mnemonic[32];
    sscanf(line, "%s", mnemonic);
    for (int i = 0; mnemonic[i]; i++) mnemonic[i] = toupper(mnemonic[i]);

    // where are operands
    char* operands = line;
    while (*operands && !isspace(*operands)) operands++;
    while (*operands && isspace(*operands)) operands++;

    if (strcmp(mnemonic, "EXIT") == 0) return 0xFFFF;  // All ones 

    // ALU operations (2 or 3 operands)
    if (strcmp(mnemonic, "AND") == 0) return parse_alu_instruction(ALU_AND, operands, error);
    if (strcmp(mnemonic, "OR") == 0) return parse_alu_instruction(ALU_OR, operands, error);
    if (strcmp(mnemonic, "XOR") == 0) return parse_alu_instruction(ALU_XOR, operands, error);
    if (strcmp(mnemonic, "NOT") == 0) {
        uint8_t dst;
        char dst_str[16];
        if (sscanf(operands, "%s", dst_str) != 1) { *error = 1; return 0; }
        dst = parse_register(dst_str);
        return encode_alu(ALU_NOT, dst, 0, 0, 0);
    }
    if (strcmp(mnemonic, "ADD") == 0) return parse_alu_instruction(ALU_ADD, operands, error);
    if (strcmp(mnemonic, "SUB") == 0) return parse_alu_instruction(ALU_SUB, operands, error);
    if (strcmp(mnemonic, "LSL") == 0) return parse_alu_instruction(ALU_LSL, operands, error);
    if (strcmp(mnemonic, "LSR") == 0) return parse_alu_instruction(ALU_LSR, operands, error);

    if (strcmp(mnemonic, "CMP") == 0) {
        uint8_t src1;
        uint16_t src2;
        char src1_str[16], src2_str[16];
        if (sscanf(operands, "%s %s", src1_str, src2_str) != 2) { *error = 1; return 0; }

        src1 = parse_register(src1_str);
        if (src1 == -1) { *error = 1; return 0; }

        if (src2_str[0] == 'X' || src2_str[0] == 'x') {
            src2 = parse_register(src2_str);
            if (src2 == -1) { *error = 1; return 0; }
            return encode_cmp(src1, src2, 0);  // REG ~ REG mode (CMP, 0x22)
        } else {
            src2 = parse_constant(src2_str);
            if (src2 == -1) { *error = 1; return 0; }
            return encode_cmp(src1, src2, 1);  // REG ~ CONST mode (CMP_I, 0x23)
        }
    }

    if (strcmp(mnemonic, "MOV") == 0) {
        uint8_t dst;
        uint16_t src;
        char dst_str[16], src_str[16];
        if (sscanf(operands, "%s %s", dst_str, src_str) != 2) { *error = 1; return 0; }
        dst = parse_register(dst_str);
        if (src_str[0] == 'X' || src_str[0] == 'x') {
            src = parse_register(src_str);
            return encode_move(dst, src, 0);  // REG = REG
        } else {
            src = parse_constant(src_str);
            return encode_move(dst, src, 1);  // REG = CONST
        }
    }

    // Conditional branch operations (B, BEQ, BLE, BLT, etc.)
    int condition = parse_branch_condition(mnemonic);
    if (condition >= 0) {
        uint16_t target;
        char target_str[16];
        if (sscanf(operands, "%s", target_str) != 1) { *error = 1; return 0; }

        // Check if target is a register
        if (target_str[0] == 'X' || target_str[0] == 'x') {
            // Parse as register - use B (0x24) with register in A field
            int reg = atoi(target_str + 1);
            if (reg < 0 || reg > 7) { *error = 1; return 0; }
            target = (uint16_t)reg;
            return encode_branch(condition, target, 0);  // B register (0x24)
        } else {
            // Parse target as label or immediate address - use B_I (0x25)
            target = atoi(target_str);
            if (target == 0 && target_str[0] != '0') {
                // Try as label
                int addr = symbol_table_lookup(symTable, target_str);
                if (addr < 0) { *error = 1; return 0; }
                target = (uint16_t)addr;
            }
            if (target > 65535) { *error = 1; return 0; }
            return encode_branch(condition, target, 1);  // B_I immediate (0x25)
        }
    }

    // READ (0x26): R[DST] = MEM[R[A]] - register addressing
    if (strcmp(mnemonic, "READ") == 0) {
        uint8_t dst, addr_reg;
        char dst_str[16], addr_str[16];
        if (sscanf(operands, "%s %s", dst_str, addr_str) != 2) { *error = 1; return 0; }
        dst = parse_register(dst_str);
        if (dst == -1) { *error = 1; return 0; }

        // Check if address is a register (READ) or constant (READ_I)
        if (addr_str[0] == 'X' || addr_str[0] == 'x') {
            // READ mode: register addressing (opcode 0x26)
            addr_reg = parse_register(addr_str);
            if (addr_reg == -1) { *error = 1; return 0; }
            return encode_read(dst, addr_reg);
        } else {
            // READ_I mode: immediate addressing (opcode 0x27)
            int addr_imm = atoi(addr_str);
            if (addr_imm < 0 || addr_imm > 15) { *error = 1; return 0; }  // 4-bit address
            return encode_read_i(dst, (uint8_t)addr_imm);
        }
    }

    // WRITE (0x28): MEM[R[B]] = R[A] - register addressing
    // WRITE_I (0x29): MEM[IMMEDIATE[3:0]] = R[A] - immediate addressing
    if (strcmp(mnemonic, "WRITE") == 0) {
        uint8_t data_reg, addr_reg;
        char data_str[16], addr_str[16];
        if (sscanf(operands, "%s %s", data_str, addr_str) != 2) { *error = 1; return 0; }
        data_reg = parse_register(data_str);
        if (data_reg == -1) { *error = 1; return 0; }

        // Check if address is a register (WRITE) or constant (WRITE_I)
        if (addr_str[0] == 'X' || addr_str[0] == 'x') {
            // WRITE mode: register addressing (opcode 0x28)
            addr_reg = parse_register(addr_str);
            if (addr_reg == -1) { *error = 1; return 0; }
            return encode_write(data_reg, addr_reg);
        } else {
            // WRITE_I mode: immediate addressing (opcode 0x29)
            int addr_imm = atoi(addr_str);
            if (addr_imm < 0 || addr_imm > 15) { *error = 1; return 0; }  // 4-bit address
            return encode_write_i(data_reg, (uint8_t)addr_imm);
        }
    }

    // PRINT: Unified PRINT instruction with auto-detection
    // Format: PRINT [ADDRESS] [CODE] - print CODE at ADDRESS
    // PRINT_REG (0x2A): SCN[R[B]] = R[A] - register address, register code
    // PRINT_REG_I (0x2B): SCN[H] = R[A] - immediate address, register code
    // PRINT_CONST (0x2C): SCN[R[B]] = K - register address, constant code
    // PRINT_CONST_I (0x2D): SCN[H] = K - immediate address, constant code
    if (strcmp(mnemonic, "PRINT") == 0) {
        char addr_str[16], code_str[16];

        // Parse operands separated by comma
        // Handle both "X0, X1" and "0, 'L'" formats
        char temp_operands[256];
        strncpy(temp_operands, operands, sizeof(temp_operands) - 1);
        temp_operands[sizeof(temp_operands) - 1] = '\0';

        // Replace comma with space for parsing
        for (int i = 0; temp_operands[i]; i++) {
            if (temp_operands[i] == ',') temp_operands[i] = ' ';
        }

        if (sscanf(temp_operands, "%s %s", addr_str, code_str) != 2) { *error = 1; return 0; }

        // Determine if address and code are registers or constants
        uint8_t is_addr_reg = (addr_str[0] == 'X' || addr_str[0] == 'x');
        uint8_t is_code_reg = (code_str[0] == 'X' || code_str[0] == 'x');

        if (is_addr_reg && is_code_reg) {
            // PRINT_REG (0x2A): both register
            uint8_t addr_reg = parse_register(addr_str);
            uint8_t code_reg = parse_register(code_str);
            if (addr_reg == -1 || code_reg == -1) { *error = 1; return 0; }
            return encode_print_reg(code_reg, addr_reg);  // encode_print_reg(data, pos)
        } else if (!is_addr_reg && is_code_reg) {
            // PRINT_REG_I (0x2B): immediate address, register code
            uint16_t addr_imm = parse_constant(addr_str);
            uint8_t code_reg = parse_register(code_str);
            if (addr_imm == -1 || code_reg == -1) { *error = 1; return 0; }
            if (addr_imm > 255) { *error = 1; return 0; }  // Address fits in 1 byte
            return encode_print_reg_i(code_reg, (uint8_t)addr_imm);  // encode_print_reg_i(data, pos)
        } else if (is_addr_reg && !is_code_reg) {
            // PRINT_CONST (0x2C): register address, constant code
            uint8_t addr_reg = parse_register(addr_str);
            uint16_t code_const = parse_constant(code_str);
            if (addr_reg == -1 || code_const == -1) { *error = 1; return 0; }
            if (code_const > 7) {
                // Constant too large for PRINT_CONST, need PRINT_CONST_I
                if (code_const > 255) { *error = 1; return 0; }
                // For PRINT_CONST_I, we need immediate address, but we have register address
                // This doesn't work - user must use immediate address for large constants
                *error = 1;
                return 0;
            }
            return encode_print_const((uint8_t)code_const, addr_reg);  // encode_print_const(data, pos)
        } else {
            // PRINT_CONST_I (0x2D): immediate address, constant code
            uint16_t addr_imm = parse_constant(addr_str);
            uint16_t code_const = parse_constant(code_str);
            if (addr_imm == -1 || code_const == -1) { *error = 1; return 0; }
            if (addr_imm > 255 || code_const > 255) { *error = 1; return 0; }
            return encode_print_const_i(code_const, (uint8_t)addr_imm);  // encode_print_const_i(data, pos)
        }
    }

    //branching
    if (mnemonic[0] == 'B') {
        uint8_t condition = B_UNCOND;
        char target_str[16];
        if (sscanf(operands, "%s", target_str) != 1) { *error = 1; return 0; }

        // determine branch
        if      (strcmp(mnemonic, "B")   == 0) condition = B_UNCOND;
        else if (strcmp(mnemonic, "BEQ") == 0) condition = B_EQ;
        else if (strcmp(mnemonic, "BNE") == 0) condition = B_NE;
        else if (strcmp(mnemonic, "BLT") == 0) condition = B_LT;
        else if (strcmp(mnemonic, "BLE") == 0) condition = B_LE;
        else if (strcmp(mnemonic, "BGT") == 0) condition = B_GT;
        else if (strcmp(mnemonic, "BGE") == 0) condition = B_GE;
        else if (strcmp(mnemonic, "BCS") == 0) condition = B_CS;
        else if (strcmp(mnemonic, "BCC") == 0) condition = B_CC;
        else if (strcmp(mnemonic, "BMI") == 0) condition = B_MI;
        else if (strcmp(mnemonic, "BPL") == 0) condition = B_PL;
        else if (strcmp(mnemonic, "BVS") == 0) condition = B_VS;
        else if (strcmp(mnemonic, "BVC") == 0) condition = B_VC;
        else if (strcmp(mnemonic, "BHI") == 0) condition = B_HI;
        else if (strcmp(mnemonic, "BLS") == 0) condition = B_LS;
        else {
            *error = 1;
            return 0;
        }

        // is target label or adr
        int addr = -1;
        if (target_str[0] >= '0' && target_str[0] <= '9')  addr = atoi(target_str);
        else                                               addr = symbol_table_lookup(symTable, target_str);
        
        if (addr < 0 || addr > 255) { *error = 1; return 0; }
        return encode_b(condition, (uint8_t)addr);
    }

    *error = 1; // invalid instruction
    return 0;
}






















static rom_format COMPILE_FORMAT = ROM_HEX;

int main(int argc, char* argv[]) {
    const char* input_file = NULL;
    const char* output_file = NULL;

    // parse args
    for (int i = 1; i < argc; i++) {
        if      (!input_file)  input_file  = argv[i];
        else if (!output_file) output_file = argv[i];
    }

    if (!input_file || !output_file) {
        fprintf(stderr, "Usage: %s <input_file> <base_name>\n", argv[0]);
        return -1;
    }
    FILE* script = fopen(input_file, "r");
    if (!script) {
        fprintf(stderr, "Error: Could not find file '%s'\n", input_file);
        return -1;
    }

    SymbolTable symTable = {0};
 
    // pass 1 : preprocessor. 
    // build label dictionary and remove comments
    char line[MAX_LINE];
    int inMultiline = 0;
    int pc = 0;  

    while (fgets(line, MAX_LINE, script) && pc < MAX_INSTR) {
        
        strip_comments(line, &inMultiline);
        char* linePtr = line;
        while (*linePtr && isspace(*linePtr)) linePtr++;
        if (!*linePtr) continue;

        //if label, log it
        if (is_label(linePtr)) {
            char labelName[32];
            parse_label(linePtr, labelName);
            symbol_table_add(&symTable, labelName, (uint8_t)pc);
        } else {
            pc++;
        }
    }

    // pass 2 : instruction compilation
    rewind(script);
    inMultiline = 0;
    uint32_t instructions[MAX_INSTR] = {0};
    int instr_count = 0;

    while (fgets(line, MAX_LINE, script) && instr_count < MAX_INSTR) {

        strip_comments(line, &inMultiline);
        int error = 0;
        uint32_t instr = parse_instruction(line, &symTable, &error);
        if (error) {
            fprintf(stderr, "Warning: Failed to parse line: %s", line);
            continue;
        }

        char* linePtr = line;
        while (*linePtr && isspace(*linePtr)) linePtr++;
        if (*linePtr && !is_label(linePtr)) {
            instructions[instr_count] = (uint32_t)instr;
            instr_count++;
        }
    }
    fclose(script);

    // Split 32-bit instructions into ALPHA (upper 16) and BETA (lower 16)
    uint16_t alpha_rom[256] = {0};
    uint16_t beta_rom[256] = {0};

    for (int i = 0; i < instr_count; i++) {
        alpha_rom[i] = (uint16_t)((instructions[i] >> 16) & 0xFFFF);
        beta_rom[i] = (uint16_t)(instructions[i] & 0xFFFF);
    }

    // Generate ALPHA filename
    char alpha_filename[512];
    snprintf(alpha_filename, 511, "%s_ALPHA.out", output_file);

    // Generate BETA filename
    char beta_filename[512];
    snprintf(beta_filename, 511, "%s_BETA.out", output_file);

    // Write ALPHA ROM
    if (write_rom_file(alpha_filename, alpha_rom, 256, COMPILE_FORMAT) < 0) {
        fprintf(stderr, "Error: Failed to write ALPHA ROM to '%s'\n", alpha_filename);
        return -1;
    }

    // Write BETA ROM
    if (write_rom_file(beta_filename, beta_rom, 256, COMPILE_FORMAT) < 0) {
        fprintf(stderr, "Error: Failed to write BETA ROM to '%s'\n", beta_filename);
        return -1;
    }

    printf("Compiled %d instructions\n", instr_count);
    printf("Generated ALPHA ROM: %s\n", alpha_filename);
    printf("Generated BETA ROM:  %s\n", beta_filename);
    return 0;
}