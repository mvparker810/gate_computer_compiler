/*

Compilation :

$ gcc lsasm.c -o lsasm

Usage :
$ ./lsasm [-f FORMAT] <input_file> <output_file>

Examples:
$ ./lsasm scripts/fib.txt machine_code.out
$ ./lsasm -f binary scripts/fib.txt machine_code.out

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#define MAX_INSTR 256
#define MAX_LINE 256

typedef enum {
    MC_HEX,
    MC_UINT,
    MC_INT,
    MC_BINARY
} machine_code_format;

typedef enum {
    ALU_AND = 0,
    ALU_OR = 1,
    ALU_XOR = 2,
    ALU_NOT = 3,
    ALU_ADD = 4,
    ALU_SUB = 5,
    ALU_LSL = 6,
    ALU_LSR = 7,
    MOVE = 8,
    MEMORY = 9,
    B = 10,
    CMP = 11,
    MEMI = 12,
    EXIT_OP = 15
} opcode;

typedef enum {
    B_UNCOND = 0,   // B (uncond)
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


#define MAX_LABELS 128
typedef struct {
    char name[32];
    uint8_t address;
} Label;
typedef struct {
    Label labels[MAX_LABELS];
    int count;
} SymbolTable;







#pragma region Label Handling

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

int is_label(const char* line) {
    while (*line && isspace(*line)) line++;
    if (!*line) return 0;
    const char* colon = strchr(line, ':');
    if (!colon) return 0;

    // labels cannot be instructions and must have alphanumeric or underscore chars
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


#pragma endregion

#pragma region Regs & Consts

// Helper function to parse register (e.g., "X0" -> 0)
uint8_t parse_register(const char* str) {
    if (str[0] != 'X' && str[0] != 'x') return -1;
    int reg = atoi(str + 1);
    if (reg < 0 || reg > 7) return -1;
    return (uint8_t)reg;
}

// Helper function to parse constant (hex, binary, or decimal)
uint8_t parse_constant(const char* str) {
    int value;
    if (str[0] == '0' && str[1]) {
        if (str[1] == 'x' || str[1] == 'X') {
            // Hexadecimal: 0xFF
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
    if (value < 0 || value > 255) return -1;
    return (uint8_t)value;
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


#pragma region Opcodes

uint16_t encode_alu(opcode op, uint8_t dst, uint8_t src1, uint8_t src2, uint8_t altBit) {
    uint16_t instr = 0;
    instr |= (op & 0xF);           // [0,4] : opcode
    if (altBit) instr |= 0x0080;   // [7]   : ALT bit
    instr |= ((dst & 0x7) << 4);   // [4,6] : destination register

    if (!altBit) {
        // REG ~ REG mode   : SRC1 [8-10], src2 [12-14]
        instr |= ((src1 & 0x7) << 8);
        instr |= ((src2 & 0x7) << 12);
    } else {
        // REG ~ CONST mode : CONST = [8,16]
        instr |= ((src1 & 0xFF) << 8);


    }
    return instr;
}

uint16_t encode_move(uint8_t dst, uint8_t src, uint8_t altBit) {
    uint16_t instr = 0;
    instr |= MOVE;                 // [0,3] : opcode (8)
    instr |= ((dst & 0x7) << 4);   // [4,6] : destination register
    if (altBit) instr |= 0x0080;   // [7]   : ALT bit

    // REG = REG mode  : src [8-10]
    if (!altBit) instr |= ((src & 0x7) << 8);    
    // REG = CONST mode: constant [8-15]
    else         instr |= ((src & 0xFF) << 8);

    return instr;
}

uint16_t encode_memory(uint8_t dst, uint8_t addr, uint8_t altBit) {
    uint16_t instr = 0;
    instr |= MEMORY;               // [0,3] : opcode (9)
    instr |= ((dst & 0x7) << 4);   // [4,6] : destination/source register
    if (altBit) instr |= 0x0080;   // [7]   : ALT bit (1 = WRITE, 0 = READ)
    instr |= ((addr & 0xFF) << 8); // [8,15]: memory address
    return instr;
}

uint16_t encode_cmp(uint8_t src1, uint8_t src2, uint8_t altBit) {
    uint16_t instr = 0;
    instr |= CMP;                       // [0,3] : opcode (11)
    instr |= ((src1 & 0x7) << 4);       // [4,6] : first source register
    if (altBit) instr |= 0x0080;        // [7]   : ALT bit

     // REG x REG mode : src2 [8-10]
    if (!altBit) instr |= ((src2 & 0x7) << 8);
    // REG x CONST mode: constant [8-15]
    else         instr |= ((src2 & 0xFF) << 8);

    return instr;
}

uint16_t encode_memi(uint8_t dataReg, uint8_t addrReg, uint8_t altBit) {
    uint16_t instr = 0;
    instr |= MEMI;                        // [0,3] : opcode (12)
    instr |= ((dataReg & 0x7) << 4);      // [4,6] : data register
    if (altBit) instr |= 0x0080;          // [7]   : ALT bit (1 = WRITE, 0 = READ)
    instr |= ((addrReg & 0x7) << 8);      // [8,10]: address register
    return instr;
}

uint16_t encode_b(uint8_t condition, uint8_t target) {
    uint16_t instr = 0;
    instr |= B;                          // [0,3] : opcode (10)
    instr |= ((condition & 0xF) << 4);   // [4,7] : branch condition
    instr |= ((target & 0xFF) << 8);     // [8,15]: target address
    return instr;
}

#pragma endregion


// Helper function to parse and encode ALU instructions
uint16_t parse_alu_instruction(opcode op, char* operands, int* error) {
    uint8_t dst, src1, src2;
    char dst_str[16], src1_str[16], src2_str[16];

    // Try 3 operands: dst src1 src2
    if (sscanf(operands, "%s %s %s", dst_str, src1_str, src2_str) == 3) {
        dst = parse_register(dst_str);
        src1 = parse_register(src1_str);
        src2 = parse_register(src2_str);
        return encode_alu(op, dst, src1, src2, 0);
    }

    // Try 2 operands: dst src1
    if (sscanf(operands, "%s %s", dst_str, src1_str) == 2) {
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
uint16_t parse_instruction(char* line, SymbolTable* symTable, int* error) {
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

    if (strcmp(mnemonic, "EXIT") == 0) return EXIT_OP; 

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
        uint8_t src1, src2;
        char src1_str[16], src2_str[16];
        if (sscanf(operands, "%s %s", src1_str, src2_str) != 2) { *error = 1; return 0; }

        src1 = parse_register(src1_str);
        if (src1 == -1) { *error = 1; return 0; }

        if (src2_str[0] == 'X' || src2_str[0] == 'x') {
            src2 = parse_register(src2_str);
            if (src2 == -1) { *error = 1; return 0; }
            return encode_cmp(src1, src2, 0);  // REG x REG mode
        } else {
            src2 = parse_constant(src2_str);
            if (src2 == -1) { *error = 1; return 0; }
            return encode_cmp(src1, src2, 1);  // REG x CONST mode
        }
    }

    if (strcmp(mnemonic, "MOV") == 0) {
        uint8_t dst, src;
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

    // MEMORY operations (READ/WRITE with MEMORY opcode 9 or MEMI opcode 12)
    if (strcmp(mnemonic, "READ") == 0) {
        uint8_t dst, addr;
        char dst_str[16], addr_str[16];
        if (sscanf(operands, "%s %s", dst_str, addr_str) != 2) { *error = 1; return 0; }
        dst = parse_register(dst_str);
        if (dst == -1) { *error = 1; return 0; }

        // Check if address is a register (MEMI) or constant (MEMORY)
        if (addr_str[0] == 'X' || addr_str[0] == 'x') {
            // MEMI mode: register addressing (opcode 12)
            addr = parse_register(addr_str);
            if (addr == -1) { *error = 1; return 0; }
            return encode_memi(dst, addr, 0);  // altBit = 0 for READ
        } else {
            // MEMORY mode: constant addressing (opcode 9)
            addr = parse_constant(addr_str);
            if (addr == -1) { *error = 1; return 0; }
            return encode_memory(dst, addr, 0);  // altBit = 0 for READ
        }
    }
    if (strcmp(mnemonic, "WRITE") == 0) {
        uint8_t src, addr;
        char src_str[16], addr_str[16];
        if (sscanf(operands, "%s %s", src_str, addr_str) != 2) { *error = 1; return 0; }
        src = parse_register(src_str);
        if (src == -1) { *error = 1; return 0; }

        // Check if address is a register (MEMI) or constant (MEMORY)
        if (addr_str[0] == 'X' || addr_str[0] == 'x') {
            // MEMI mode: register addressing (opcode 12)
            addr = parse_register(addr_str);
            if (addr == -1) { *error = 1; return 0; }
            return encode_memi(src, addr, 1);  // altBit = 1 for WRITE
        } else {
            // MEMORY mode: constant addressing (opcode 9)
            addr = parse_constant(addr_str);
            if (addr == -1) { *error = 1; return 0; }
            return encode_memory(src, addr, 1);  // altBit = 1 for WRITE
        }
    }

    //branching
    if (mnemonic[0] == 'B') {
        uint8_t target, condition = B_UNCOND;
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

static machine_code_format COMPILE_FORMAT = MC_HEX;

int main(int argc, char* argv[]) {
    const char* input_file = NULL;
    const char* output_file = NULL;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -f requires an argument\n");
                return -1;
            }
            const char* format = argv[++i];
            if (strcmp(format, "hex") == 0) COMPILE_FORMAT = MC_HEX;
            else if (strcmp(format, "uint") == 0) COMPILE_FORMAT = MC_UINT;
            else if (strcmp(format, "int") == 0) COMPILE_FORMAT = MC_INT;
            else if (strcmp(format, "binary") == 0) COMPILE_FORMAT = MC_BINARY;
            else {
                fprintf(stderr, "Error: Unknown format '%s'\n", format);
                return -1;
            }
        } else if (argv[i][0] != '-') {
            // Positional arguments
            if (!input_file) input_file = argv[i];
            else if (!output_file) output_file = argv[i];
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            return -1;
        }
    }

    if (!input_file || !output_file) {
        fprintf(stderr, "Usage: %s [-f FORMAT] <input_file> <output_file>\n", argv[0]);
        return -1;
    }

    FILE* script = fopen(input_file, "r");
    if (!script) {
        fprintf(stderr, "Error: File '%s' not found\n", input_file);
        return -1;
    }

    SymbolTable symTable;
    symbol_table_init(&symTable);

    // pass 1 : label scan
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
    uint16_t instructions[MAX_INSTR] = {0};
    int instr_count = 0;

    while (fgets(line, MAX_LINE, script) && instr_count < MAX_INSTR) {
       
        strip_comments(line, &inMultiline);
        int error = 0;
        uint16_t instr = parse_instruction(line, &symTable, &error);
        if (error) {
            fprintf(stderr, "Warning: Failed to parse line: %s", line);
            continue;
        }
        
        char* linePtr = line;
        while (*linePtr && isspace(*linePtr)) linePtr++;
        if (*linePtr && !is_label(linePtr)) {
            instructions[instr_count] = instr;
            instr_count++;
        }
    }
    fclose(script);

    FILE* OUT = fopen(output_file, "w");
    if (!OUT) {
        fprintf(stderr, "Error: Failed to open '%s' for writing\n", output_file);
        return -1;
    }

    for (int i = 0; i < instr_count; i++) {
        switch (COMPILE_FORMAT) {
            case MC_HEX:
                fprintf(OUT, "%04X\n", instructions[i]);
            break;
            case MC_UINT:
                fprintf(OUT, "%u\n", instructions[i]);
            break;
            case MC_INT:
                fprintf(OUT, "%d\n", (int16_t)instructions[i]);
            break;
            case MC_BINARY:
                for (int b = 15; b >= 0; b--) {
                    fputc((instructions[i] >> b) & 1 ? '1' : '0', OUT);
                }
                fputc('\n', OUT);
            break;
        }
    }

    fclose(OUT);
    printf("Compiled %d instructions to '%s'\n", instr_count, output_file);
    return 0;
}