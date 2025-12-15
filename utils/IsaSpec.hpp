#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace IsaSpec {

// Instruction type enumeration
enum class InstructionType {
    TYPE_ALU = 0,
    TYPE_FPU = 1,
    TYPE_MOVE = 2,
    TYPE_CMP = 3,
    TYPE_BRANCH = 4,
    TYPE_MEMORY = 5,
    TYPE_PRINT_REG = 6,
    TYPE_PRINT_CONST = 7,
    TYPE_SERVICE = 8
};

// Instruction format enumeration
enum class Format {
    R,   // Register Format
    I,   // Immediate Format
    J,   // Jump Format
    JI   // Jump Immediate Format
};

// Instruction flags structure
struct InstructionFlags {
    bool VALID = false;
    bool TRY_WRITE = false;
    bool TRY_READ_A = false;
    bool TRY_READ_B = false;
    bool OVERRIDE_B = false;
    bool OVERRIDE_WRITE = false;
    bool IMMEDIATE = false;
};

// Technical instruction definition (for assembler/execution)
struct InstructionTech {
    std::string technical_name;  // e.g., "ALU_AND", "ALU_AND_I"
    std::string mnemonic;         // e.g., "AND"
    uint8_t opcode;
    Format format;
    InstructionType type;
    InstructionFlags flags;

    InstructionTech(const std::string& tech_name, const std::string& mnem, uint8_t op,
                    Format fmt, InstructionType typ, InstructionFlags flg)
        : technical_name(tech_name), mnemonic(mnem), opcode(op), format(fmt),
          type(typ), flags(flg) {}
};

// Documentation info (for humans/documentation generation)
struct InstructionDoc {
    std::string technical_name;   // Links to InstructionTech
    std::string description;      // e.g., "R[DST] = R[A] & R[B]"
    std::string usage_example;    // e.g., "AND X0, X1, X2"
    std::string explanation;      // e.g., "Bitwise AND of X1 and X2, store in X0"

    InstructionDoc(const std::string& tech_name, const std::string& desc,
                   const std::string& example, const std::string& explain)
        : technical_name(tech_name), description(desc), usage_example(example),
          explanation(explain) {}
};

// Branch condition definition
struct BranchCondition {
    std::string mnemonic;
    uint8_t code;
    std::string name;
    std::string description;

    BranchCondition(const std::string& mnem, uint8_t c, const std::string& n, const std::string& desc)
        : mnemonic(mnem), code(c), name(n), description(desc) {}
};

// Main ISA specification structure
struct ISA_SPEC {
    std::string version;

    // Architecture parameters
    struct {
        int instruction_width = 32;
        int register_count = 8;
        int register_width = 16;
        int memory_size = 256;
        int memory_width = 16;
    } architecture;

    // Technical table - for assembler/execution
    std::vector<InstructionTech> instructions_tech;
    std::map<uint8_t, const InstructionTech*> opcode_map;
    std::map<std::string, const InstructionTech*> tech_name_map;

    // Documentation table - for humans/docs
    std::vector<InstructionDoc> instructions_doc;

    std::vector<BranchCondition> branch_conditions;
};

// Algorithmic ISA spec generator
inline ISA_SPEC generateISASpec() {
    ISA_SPEC spec;
    spec.version = "2.0";

    // ALU instruction flags for register format
    InstructionFlags aluRegFlags = {
        .VALID = true,
        .TRY_WRITE = true,
        .TRY_READ_A = true,
        .TRY_READ_B = true,
        .OVERRIDE_B = false,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = false
    };

    // ALU instruction flags for immediate format
    InstructionFlags aluImmFlags = {
        .VALID = true,
        .TRY_WRITE = true,
        .TRY_READ_A = true,
        .TRY_READ_B = false,
        .OVERRIDE_B = true,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = true
    };

    // ALU Operations - Modular definition (0x00-0x0B for R, 0x10-0x1B for I)
    struct ALUDef {
        const char* tech_suffix;
        const char* mnemonic;
        const char* desc_reg;
        const char* desc_imm;
        const char* usage_reg;
        const char* usage_imm;
        const char* explain_reg;
        const char* explain_imm;
    };

    ALUDef aluOps[] = {
        {"AND", "AND", "R[DST] = R[A] & R[B]", "R[DST] = R[A] & IMM", "AND X0, X1, X2", "AND X0, X1, 0xFF", "Bitwise AND of X1 and X2, store in X0", "Bitwise AND of X1 and 0xFF, store in X0"},
        {"OR", "OR", "R[DST] = R[A] | R[B]", "R[DST] = R[A] | IMM", "OR X0, X1, X2", "OR X0, X1, 0x10", "Bitwise OR of X1 and X2, store in X0", "Bitwise OR of X1 and 0x10, store in X0"},
        {"XOR", "XOR", "R[DST] = R[A] ^ R[B]", "R[DST] = R[A] ^ IMM", "XOR X0, X1, X2", "XOR X0, X1, 0xFFFF", "Bitwise XOR of X1 and X2, store in X0", "Bitwise XOR of X1 and 0xFFFF, store in X0"},
        {"NOT", "NOT", "R[DST] = ~R[A]", "R[DST] = ~R[A]", "NOT X0", "NOT X0", "Bitwise NOT of X0, store in X0", "Bitwise NOT of X0, store in X0"},
        {"ADD", "ADD", "R[DST] = R[A] + R[B]", "R[DST] = R[A] + IMM", "ADD X0, X1, X2", "ADD X0, X1, 5", "Add X1 and X2, store sum in X0", "Add X1 and 5, store sum in X0"},
        {"SUB", "SUB", "R[DST] = R[A] - R[B]", "R[DST] = R[A] - IMM", "SUB X0, X1, X2", "SUB X0, X1, 10", "Subtract X2 from X1, store in X0", "Subtract 10 from X1, store in X0"},
        {"LSL", "LSL", "R[DST] = R[A] << R[B]", "R[DST] = R[A] << IMM", "LSL X0, X1, X2", "LSL X0, X1, 3", "Shift X1 left by X2 bits, store in X0", "Shift X1 left by 3 bits, store in X0"},
        {"LSR", "LSR", "R[DST] = R[A] >> R[B]", "R[DST] = R[A] >> IMM", "LSR X0, X1, X2", "LSR X0, X1, 2", "Shift X1 right by X2 bits, store in X0", "Shift X1 right by 2 bits, store in X0"},
        {"BCDL", "BCDL", "R[DST] = BCD_LOW(R[A])", "R[DST] = BCD_LOW(R[A])", "BCDL X0, X1", "BCDL X0, X1", "Convert X1 to BCD, extract lower 4 digits to X0", "Convert X1 to BCD, extract lower 4 digits to X0"},
        {"BCDH", "BCDH", "R[DST] = BCD_HIGH(R[A])", "R[DST] = BCD_HIGH(R[A])", "BCDH X0, X1", "BCDH X0, X1", "Convert X1 to BCD, extract upper 4 digits to X0", "Convert X1 to BCD, extract upper 4 digits to X0"},
        {"UMUL_L", "UMUL_L", "R[DST] = LOW(R[A] * R[B]) (unsigned)", "R[DST] = LOW(R[A] * IMM) (unsigned)", "UMUL_L X0, X1, X2", "UMUL_L X0, X1, 3", "Unsigned multiply X1 by X2, store lower 16 bits in X0", "Unsigned multiply X1 by 3, store lower 16 bits in X0"},
        {"UMUL_H", "UMUL_H", "R[DST] = HIGH(R[A] * R[B]) (unsigned)", "R[DST] = HIGH(R[A] * IMM) (unsigned)", "UMUL_H X0, X1, X2", "UMUL_H X0, X1, 3", "Unsigned multiply X1 by X2, store upper 16 bits in X0", "Unsigned multiply X1 by 3, store upper 16 bits in X0"},
        {"MUL_L", "MUL_L", "R[DST] = LOW(R[A] * R[B]) (signed)", "R[DST] = LOW(R[A] * IMM) (signed)", "MUL_L X0, X1, X2", "MUL_L X0, X1, -2", "Signed multiply X1 by X2, store lower 16 bits in X0", "Signed multiply X1 by -2, store lower 16 bits in X0"},
        {"MUL_H", "MUL_H", "R[DST] = HIGH(R[A] * R[B]) (signed)", "R[DST] = HIGH(R[A] * IMM) (signed)", "MUL_H X0, X1, X2", "MUL_H X0, X1, -2", "Signed multiply X1 by X2, store upper 16 bits in X0", "Signed multiply X1 by -2, store upper 16 bits in X0"},
        {"NUL0E", "NUL0E", "Reserved ALU 0x0E", "Reserved ALU 0x1E", "NUL0E", "NUL0E", "Reserved for future ALU operation", "Reserved for future ALU operation"},
        {"NUL0F", "NUL0F", "Reserved ALU 0x0F", "Reserved ALU 0x1F", "NUL0F", "NUL0F", "Reserved for future ALU operation", "Reserved for future ALU operation"}
    };

    // Generate ALU instructions algorithmically
    for (int i = 0; i < 16; i++) {
        // Register format (0x00 + i)
        std::string techNameReg = std::string("ALU_") + aluOps[i].tech_suffix;
        spec.instructions_tech.emplace_back(techNameReg, aluOps[i].mnemonic, 0x00 + i, Format::R, InstructionType::TYPE_ALU, aluRegFlags);
        spec.instructions_doc.emplace_back(techNameReg, aluOps[i].desc_reg, aluOps[i].usage_reg, aluOps[i].explain_reg);

        // Immediate format (0x10 + i)
        std::string techNameImm = std::string("ALU_") + aluOps[i].tech_suffix + "_I";
        spec.instructions_tech.emplace_back(techNameImm, aluOps[i].mnemonic, 0x10 + i, Format::I, InstructionType::TYPE_ALU, aluImmFlags);
        spec.instructions_doc.emplace_back(techNameImm, aluOps[i].desc_imm, aluOps[i].usage_imm, aluOps[i].explain_imm);
    }


    InstructionFlags fpuRegFlags = {
        .VALID = true,
        .TRY_WRITE = true,
        .TRY_READ_A = true,
        .TRY_READ_B = true,
        .OVERRIDE_B = false,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = false
    };

    InstructionFlags fpuImmFlags = {
        .VALID = true,
        .TRY_WRITE = true,
        .TRY_READ_A = true,
        .TRY_READ_B = false,
        .OVERRIDE_B = true,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = true
    };

    // FPU placeholder operations (16 operations, same as ALU)
    for (int i = 0; i < 16; i++) {
        // Register format (0x20 + i)
        std::string techNameReg = "FPU_NUL" + std::to_string(0x20 + i);
        std::string mnemonicReg = "FNUL" + std::to_string(i);
        spec.instructions_tech.emplace_back(techNameReg, mnemonicReg, 0x20 + i, Format::R, InstructionType::TYPE_FPU, fpuRegFlags);
        spec.instructions_doc.emplace_back(techNameReg, "Reserved FPU operation", mnemonicReg, "Reserved for future floating-point operation");

        // Immediate format (0x30 + i)
        std::string techNameImm = "FPU_NUL" + std::to_string(0x30 + i) + "_I";
        std::string mnemonicImm = "FNUL" + std::to_string(i);
        spec.instructions_tech.emplace_back(techNameImm, mnemonicImm, 0x30 + i, Format::I, InstructionType::TYPE_FPU, fpuImmFlags);
        spec.instructions_doc.emplace_back(techNameImm, "Reserved FPU operation (immediate)", mnemonicImm, "Reserved for future floating-point operation");
    }

    // Move Operations
    InstructionFlags moveRegFlags = {
        .VALID = true,
        .TRY_WRITE = true,
        .TRY_READ_A = true,
        .TRY_READ_B = true,
        .OVERRIDE_B = false,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = false
    };
    InstructionFlags moveImmFlags = {
        .VALID = true,
        .TRY_WRITE = true,
        .TRY_READ_A = true,
        .TRY_READ_B = false,
        .OVERRIDE_B = true,
        .OVERRIDE_WRITE = true,
        .IMMEDIATE = true
    };
    // Technical table
    spec.instructions_tech.emplace_back("MOVE", "MOV", 0x40, Format::R, InstructionType::TYPE_MOVE, moveRegFlags);
    spec.instructions_tech.emplace_back("MOVE_I", "MOV", 0x41, Format::I, InstructionType::TYPE_MOVE, moveImmFlags);

    // Documentation table
    spec.instructions_doc.emplace_back("MOVE", "R[DST] = R[SRC]", "MOV X0, X1", "Copy value from X1 to X0");
    spec.instructions_doc.emplace_back("MOVE_I", "R[DST] = IMM", "MOV X0, 100", "Load immediate value 100 into X0");

    // Comparison Operations
    InstructionFlags cmpRegFlags = {
        .VALID = true,
        .TRY_WRITE = false,
        .TRY_READ_A = true,
        .TRY_READ_B = true,
        .OVERRIDE_B = false,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = false
    };
    InstructionFlags cmpImmFlags = {
        .VALID = true,
        .TRY_WRITE = false,
        .TRY_READ_A = true,
        .TRY_READ_B = false,
        .OVERRIDE_B = true,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = true
    };
    // Technical table
    spec.instructions_tech.emplace_back("CMP", "CMP", 0x42, Format::R, InstructionType::TYPE_CMP, cmpRegFlags);
    spec.instructions_tech.emplace_back("CMP_I", "CMP", 0x43, Format::I, InstructionType::TYPE_CMP, cmpImmFlags);

    // Documentation table
    spec.instructions_doc.emplace_back("CMP", "FLAGS = R[A] ~ R[B]", "CMP X0, X1", "Compare X0 and X1, set condition flags");
    spec.instructions_doc.emplace_back("CMP_I", "FLAGS = R[A] ~ IMM", "CMP X0, 42", "Compare X0 and 42, set condition flags");

    // Branch Operations
    InstructionFlags branchRegFlags = {
        .VALID = true,
        .TRY_WRITE = false,
        .TRY_READ_A = false,
        .TRY_READ_B = true,
        .OVERRIDE_B = false,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = false
    };
    InstructionFlags branchImmFlags = {
        .VALID = true,
        .TRY_WRITE = false,
        .TRY_READ_A = false,
        .TRY_READ_B = false,
        .OVERRIDE_B = true,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = true
    };
    // Technical table
    spec.instructions_tech.emplace_back("BRANCH", "B", 0x44, Format::J, InstructionType::TYPE_BRANCH, branchRegFlags);
    spec.instructions_tech.emplace_back("BRANCH_I", "B", 0x45, Format::JI, InstructionType::TYPE_BRANCH, branchImmFlags);

    // Documentation table
    spec.instructions_doc.emplace_back("BRANCH", "CONDITION => PC = R[A]", "BEQ X0", "If condition EQ is true, jump to address in X0");
    spec.instructions_doc.emplace_back("BRANCH_I", "CONDITION => PC = IMM", "BNE 100", "If condition NE is true, jump to address 100");

    // Memory Operations
    InstructionFlags readRegFlags = {
        .VALID = true,
        .TRY_WRITE = true,
        .TRY_READ_A = false,
        .TRY_READ_B = true,
        .OVERRIDE_B = false,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = false
    };
    InstructionFlags readImmFlags = {
        .VALID = true,
        .TRY_WRITE = true,
        .TRY_READ_A = false,
        .TRY_READ_B = false,
        .OVERRIDE_B = true,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = true
    };
    // Technical table
    spec.instructions_tech.emplace_back("READ", "READ", 0x46, Format::R, InstructionType::TYPE_MEMORY, readRegFlags);
    spec.instructions_tech.emplace_back("READ_I", "READ", 0x47, Format::I, InstructionType::TYPE_MEMORY, readImmFlags);

    // Documentation table
    spec.instructions_doc.emplace_back("READ", "R[DST] = MEM[R[B]]", "READ X0, X1", "Load value from memory address in X1 into X0");
    spec.instructions_doc.emplace_back("READ_I", "R[DST] = MEM[IMM]", "READ X0, 50", "Load value from memory address 50 into X0");

    InstructionFlags writeRegFlags = {
        .VALID = true,
        .TRY_WRITE = false,
        .TRY_READ_A = true,
        .TRY_READ_B = true,
        .OVERRIDE_B = false,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = false
    };
    InstructionFlags writeImmFlags = {
        .VALID = true,
        .TRY_WRITE = false,
        .TRY_READ_A = true,
        .TRY_READ_B = false,
        .OVERRIDE_B = true,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = true
    };
    // Technical table
    spec.instructions_tech.emplace_back("WRITE", "WRITE", 0x48, Format::R, InstructionType::TYPE_MEMORY, writeRegFlags);
    spec.instructions_tech.emplace_back("WRITE_I", "WRITE", 0x49, Format::I, InstructionType::TYPE_MEMORY, writeImmFlags);

    // Documentation table
    spec.instructions_doc.emplace_back("WRITE", "MEM[R[B]] = R[A]", "WRITE X0, X1", "Store value from X0 to memory address in X1");
    spec.instructions_doc.emplace_back("WRITE_I", "MEM[IMM] = R[A]", "WRITE X0, 50", "Store value from X0 to memory address 50");

    // Print Operations
    InstructionFlags printRegRegFlags = {
        .VALID = true,
        .TRY_WRITE = false,
        .TRY_READ_A = true,
        .TRY_READ_B = true,
        .OVERRIDE_B = false,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = false
    };
    InstructionFlags printRegImmFlags = {
        .VALID = true,
        .TRY_WRITE = false,
        .TRY_READ_A = true,
        .TRY_READ_B = false,
        .OVERRIDE_B = true,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = true
    };
    // Technical table
    spec.instructions_tech.emplace_back("PRINT_REG", "PRINT", 0x4A, Format::R, InstructionType::TYPE_PRINT_REG, printRegRegFlags);
    spec.instructions_tech.emplace_back("PRINT_REG_I", "PRINT", 0x4B, Format::I, InstructionType::TYPE_PRINT_REG, printRegImmFlags);

    // Documentation table
    spec.instructions_doc.emplace_back("PRINT_REG", "SCN[R[B]] = R[A]", "PRINT X1, X0", "Print value in X1 at screen position in X0");
    spec.instructions_doc.emplace_back("PRINT_REG_I", "SCN[IMM] = R[A]", "PRINT 10, X0", "Print value in X0 at screen position 10");

    InstructionFlags printConstRegFlags = {
        .VALID = true,
        .TRY_WRITE = false,
        .TRY_READ_A = false,
        .TRY_READ_B = true,
        .OVERRIDE_B = false,
        .OVERRIDE_WRITE = true,
        .IMMEDIATE = false
    };
    InstructionFlags printConstImmFlags = {
        .VALID = true,
        .TRY_WRITE = false,
        .TRY_READ_A = false,
        .TRY_READ_B = false,
        .OVERRIDE_B = true,
        .OVERRIDE_WRITE = true,
        .IMMEDIATE = true
    };
    // Technical table
    spec.instructions_tech.emplace_back("PRINT_CNS", "PRINT", 0x4C, Format::R, InstructionType::TYPE_PRINT_CONST, printConstRegFlags);
    spec.instructions_tech.emplace_back("PRINT_CNS_I", "PRINT", 0x4D, Format::I, InstructionType::TYPE_PRINT_CONST, printConstImmFlags);

    // Documentation table
    spec.instructions_doc.emplace_back("PRINT_CNS", "SCN[R[B]] = CONST", "PRINT X0, 'A'", "Print ASCII 'A' at screen position in X0");
    spec.instructions_doc.emplace_back("PRINT_CNS_I", "SCN[IMM] = CONST", "PRINT 5, 'H'", "Print ASCII 'H' at screen position 5");

    // Service Operations
    InstructionFlags exitFlags = {
        .VALID = true,
        .TRY_WRITE = false,
        .TRY_READ_A = false,
        .TRY_READ_B = false,
        .OVERRIDE_B = false,
        .OVERRIDE_WRITE = false,
        .IMMEDIATE = false
    };
    // Technical table
    spec.instructions_tech.emplace_back("EXIT", "EXIT", 0xFF, Format::R, InstructionType::TYPE_SERVICE, exitFlags);

    // Documentation table
    spec.instructions_doc.emplace_back("EXIT", "Halt execution", "EXIT", "Stop program execution");

    // Build opcode lookup maps
    for (const auto& instr : spec.instructions_tech) {
        spec.opcode_map[instr.opcode] = &instr;
        spec.tech_name_map[instr.technical_name] = &instr;
    }

    // Branch Conditions (algorithmically generated)
    spec.branch_conditions.emplace_back("B", 0, "Unconditional", "Always branch");
    spec.branch_conditions.emplace_back("BEQ", 1, "Equal", "Branch if equal (Z set)");
    spec.branch_conditions.emplace_back("BNE", 2, "Not Equal", "Branch if not equal (Z clear)");
    spec.branch_conditions.emplace_back("BLT", 3, "Less Than", "Branch if less than (N set)");
    spec.branch_conditions.emplace_back("BLE", 4, "Less or Equal", "Branch if less than or equal (N set or Z set)");
    spec.branch_conditions.emplace_back("BGT", 5, "Greater Than", "Branch if greater than (N clear and Z clear)");
    spec.branch_conditions.emplace_back("BGE", 6, "Greater or Equal", "Branch if greater than or equal (N clear)");
    spec.branch_conditions.emplace_back("BCS", 7, "Carry Set", "Branch if carry set (C set)");
    spec.branch_conditions.emplace_back("BCC", 8, "Carry Clear", "Branch if carry clear (C clear)");
    spec.branch_conditions.emplace_back("BMI", 9, "Minus", "Branch if negative (N set)");
    spec.branch_conditions.emplace_back("BPL", 10, "Plus", "Branch if positive (N clear)");
    spec.branch_conditions.emplace_back("BVS", 11, "Overflow Set", "Branch if overflow set (V set)");
    spec.branch_conditions.emplace_back("BVC", 12, "Overflow Clear", "Branch if overflow clear (V clear)");
    spec.branch_conditions.emplace_back("BHI", 13, "Higher", "Branch if higher (unsigned)");
    spec.branch_conditions.emplace_back("BLS", 14, "Lower or Same", "Branch if lower or same (unsigned)");

    return spec;
}

} // namespace IsaSpec
