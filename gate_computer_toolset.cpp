/*

why is output format a string. make it a enum const

Gate Computer Toolset
Interactive CLI tool for assembling code and generating ROMs

Compilation:
   g++ -std=c++17 -Wall -o gct gate_computer_toolset.cpp
Usage:
  ./gct

*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>
#include <functional>
#include <cctype>
#include "utils/RomWriter.hpp"
#include "utils/IsaSpec.hpp"

// Tool Registry - holds all registered tools
class ToolRegistry {
private:
    static std::vector<class Tool*>& getTools() {
        static std::vector<Tool*> tools;
        return tools;
    }

public:
    static void registerTool(Tool* tool) {
        getTools().push_back(tool);
    }

    static const std::vector<Tool*>& getAllTools() {
        return getTools();
    }
};

// Base class for all tools
class Tool {
public:
    std::string name;           // Display name shown in menu
    std::string description;    // Description of what it does

    Tool(const std::string& n, const std::string& desc)
        : name(n), description(desc) {}

    virtual ~Tool() = default;

    // Execute the tool - must be implemented by each tool
    virtual void execute(RomFormat outputFormat) = 0;

    // Get user inputs (override if tool needs specific inputs)
    virtual void getInputs() {}
};

// Simple base class for tools
template<typename T>
class AutoRegisterTool : public Tool {
public:
    AutoRegisterTool(const std::string& n, const std::string& desc)
        : Tool(n, desc) {}
};

// ============================================
// DIGITAL LOGIC SIM HELPER
// ============================================

class DigitalLogicSimHelper {
private:
    std::string chipName;
    std::string basePath;

public:
    DigitalLogicSimHelper(const std::string& chip = "16-CPU")
        : chipName(chip),
          basePath("C:\\Users\\Limey\\AppData\\LocalLow\\SebastianLague\\Digital-Logic-Sim\\Projects\\16-Bit Computer 1.3\\Chips\\") {}

    // Update a single subchip's InternalData array
    bool updateSubchipData(const std::string& subchipLabel, const std::vector<uint16_t>& data) {
        std::string jsonPath = basePath + chipName + ".json";

        // Read entire file
        std::ifstream jsonFile(jsonPath);
        if (!jsonFile.is_open()) {
            std::cerr << "Warning: Could not open Digital Logic Sim file: " << jsonPath << "\n";
            return false;
        }

        std::string fileContent((std::istreambuf_iterator<char>(jsonFile)), std::istreambuf_iterator<char>());
        jsonFile.close();

        // Find the subchip by label
        std::string searchStr = "\"Label\":\"" + subchipLabel + "\"";
        size_t labelPos = fileContent.find(searchStr);
        if (labelPos == std::string::npos) {
            std::cerr << "Warning: Could not find subchip with label '" << subchipLabel << "'\n";
            return false;
        }

        // Find InternalData array start after this label
        size_t dataStart = fileContent.find("\"InternalData\":[", labelPos);
        if (dataStart == std::string::npos) {
            std::cerr << "Warning: Could not find InternalData for '" << subchipLabel << "'\n";
            return false;
        }

        size_t arrayStart = dataStart + 16; // Skip "InternalData":[
        size_t arrayEnd = fileContent.find("]", arrayStart);

        // Build new array string
        std::string newArray;
        for (size_t i = 0; i < data.size(); i++) {
            if (i > 0) newArray += ",";
            newArray += std::to_string(data[i]);
        }

        // Replace old array with new
        fileContent.replace(arrayStart, arrayEnd - arrayStart, newArray);

        // Write back to file
        std::ofstream outFile(jsonPath);
        if (!outFile.is_open()) {
            std::cerr << "Error: Could not write to Digital Logic Sim file\n";
            return false;
        }
        outFile << fileContent;
        outFile.close();

        return true;
    }

    // Update multiple subchips at once
    bool updateMultipleSubchips(const std::vector<std::pair<std::string, std::vector<uint16_t>>>& updates) {
        std::string jsonPath = basePath + chipName + ".json";

        // Read entire file
        std::ifstream jsonFile(jsonPath);
        if (!jsonFile.is_open()) {
            std::cerr << "Warning: Could not open Digital Logic Sim file: " << jsonPath << "\n";
            return false;
        }

        std::string fileContent((std::istreambuf_iterator<char>(jsonFile)), std::istreambuf_iterator<char>());
        jsonFile.close();

        // Process each update
        for (const auto& update : updates) {
            const std::string& subchipLabel = update.first;
            const std::vector<uint16_t>& data = update.second;

            // Find the subchip by label
            std::string searchStr = "\"Label\":\"" + subchipLabel + "\"";
            size_t labelPos = fileContent.find(searchStr);
            if (labelPos == std::string::npos) {
                std::cerr << "Warning: Could not find subchip with label '" << subchipLabel << "'\n";
                continue;
            }

            // Find InternalData array start after this label
            size_t dataStart = fileContent.find("\"InternalData\":[", labelPos);
            if (dataStart == std::string::npos) {
                std::cerr << "Warning: Could not find InternalData for '" << subchipLabel << "'\n";
                continue;
            }

            size_t arrayStart = dataStart + 16; // Skip "InternalData":[
            size_t arrayEnd = fileContent.find("]", arrayStart);

            // Build new array string
            std::string newArray;
            for (size_t i = 0; i < data.size(); i++) {
                if (i > 0) newArray += ",";
                newArray += std::to_string(data[i]);
            }

            // Replace old array with new
            fileContent.replace(arrayStart, arrayEnd - arrayStart, newArray);
        }

        // Write back to file
        std::ofstream outFile(jsonPath);
        if (!outFile.is_open()) {
            std::cerr << "Error: Could not write to Digital Logic Sim file\n";
            return false;
        }
        outFile << fileContent;
        outFile.close();

        std::cout << "Updated Digital Logic Sim chip '" << chipName << "' in: " << jsonPath << "\n";
        return true;
    }
};

// ============================================
// TOOL DEFINITIONS
// ============================================

// Assembler Tool
class AssemblerTool : public AutoRegisterTool<AssemblerTool> {
private:
    std::string inputFile;
    std::string outputBase;
    IsaSpec::ISA_SPEC isaSpec;

    // Helper: Check if mnemonic is an ALU operation (based on type in spec)
    bool isAluOperation(const std::string& mnemonic) {
        // Search through technical instructions for matching mnemonic with TYPE_ALU
        for (const auto& instr : isaSpec.instructions_tech) {
            if (instr.mnemonic == mnemonic && instr.type == IsaSpec::InstructionType::TYPE_ALU) {
                return true;
            }
        }
        return false;
    }

    // Helper: Find opcode from mnemonic and operand types using ISA spec
    // The compiler differentiates by checking if the last operand is a register or immediate
    uint8_t findOpcode(const std::string& mnemonic, bool immediate) {
        for (const auto& instr : isaSpec.instructions_tech) {
            if (instr.mnemonic == mnemonic && instr.flags.IMMEDIATE == immediate) {
                return instr.opcode;
            }
        }
        return 0xFF; // Invalid opcode
    }

    // Helper: Find opcode from mnemonic, type, and immediate flag
    uint8_t findOpcodeByType(const std::string& mnemonic, IsaSpec::InstructionType type, bool immediate) {
        for (const auto& instr : isaSpec.instructions_tech) {
            if (instr.mnemonic == mnemonic && instr.type == type && instr.flags.IMMEDIATE == immediate) {
                return instr.opcode;
            }
        }
        return 0xFF; // Invalid opcode
    }

    // Helper: Find branch condition code from mnemonic
    int findBranchCondition(const std::string& mnemonic) {
        for (const auto& branch : isaSpec.branch_conditions) {
            if (branch.mnemonic == mnemonic) {
                return branch.code;
            }
        }
        return -1;
    }

    // Symbol table for labels
    struct Label {
        std::string name;
        uint8_t address;
    };
    std::vector<Label> symbolTable;

    // Alias table for register aliasing
    struct RegisterAlias {
        std::string alias;
        std::string registerName;  // e.g., "X0", "X1"
    };
    std::vector<RegisterAlias> aliasTable;

    // Helper: Validate alias name (alphanumeric + underscore only)
    bool isValidAliasName(const std::string& name) {
        if (name.empty()) return false;

        // Check that name contains only alphanumeric characters and underscores
        for (char c : name) {
            if (!std::isalnum(c) && c != '_') {
                return false;
            }
        }

        // Check that alias doesn't conflict with instruction mnemonics
        if (isAluOperation(name)) return false;

        // Check against other instruction mnemonics
        const char* reserved[] = {
            "MOV", "CMP", "B", "BEQ", "BNE", "BLT", "BLE", "BGT", "BGE",
            "BCS", "BCC", "BMI", "BPL", "BVS", "BVC", "BHI", "BLS",
            "READ", "WRITE", "PRINT", "EXIT", "NOT"
        };

        for (const char* mnemonic : reserved) {
            if (name == mnemonic) return false;
        }

        return true;
    }

    // Helper: Resolve alias to register name
    std::string resolveAlias(const std::string& str) {
        // Check if this is an alias
        for (const auto& alias : aliasTable) {
            if (alias.alias == str) {
                return alias.registerName;
            }
        }
        // Not an alias, return original
        return str;
    }

    // Helper: Parse register (e.g., "X0" -> 0), with alias support
    int parseRegister(const std::string& str) {
        // First check if it's an alias
        std::string resolved = resolveAlias(str);

        if (resolved.length() < 2) return -1;
        if (resolved[0] != 'X' && resolved[0] != 'x') return -1;
        int reg = std::atoi(resolved.c_str() + 1);
        if (reg < 0 || reg > 7) return -1;
        return reg;
    }

    // Helper: Parse constant (hex, binary, decimal, ASCII)
    int parseConstant(const std::string& str) {
        if (str.empty()) return -1;

        // ASCII character literal: 'A' -> 65
        if (str.length() == 3 && str[0] == '\'' && str[2] == '\'') {
            return (int)(unsigned char)str[1];
        }

        int value;
        if (str.length() > 2 && str[0] == '0') {
            if (str[1] == 'x' || str[1] == 'X') {
                // Hexadecimal
                value = std::strtol(str.c_str(), nullptr, 16);
            } else if (str[1] == 'b' || str[1] == 'B') {
                // Binary
                value = std::strtol(str.c_str() + 2, nullptr, 2);
            } else {
                // Decimal
                value = std::atoi(str.c_str());
            }
        } else {
            // Decimal
            value = std::atoi(str.c_str());
        }

        if (value < 0 || value > 65535) return -1;
        return value;
    }

    // Helper: Strip comments from line
    void stripComments(std::string& line, bool& inMultiline) {
        std::string result;
        size_t i = 0;
        size_t len = line.length();

        while (i < len) {
            if (inMultiline) {
                // Look for end of multiline comment */
                if (i < len - 1 && line[i] == '*' && line[i+1] == '/') {
                    inMultiline = false;
                    i += 2;
                } else {
                    i++;
                }
            } else {
                // Check for start of multiline comment /*
                if (i < len - 1 && line[i] == '/' && line[i+1] == '*') {
                    inMultiline = true;
                    i += 2;
                }
                // Check for single-line comment //
                else if (i < len - 1 && line[i] == '/' && line[i+1] == '/') {
                    break; // Rest of line is comment
                }
                // Regular character
                else {
                    result += line[i++];
                }
            }
        }
        line = result;
    }

    // Helper: Check if line is a label
    bool isLabel(const std::string& line) {
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) return false;

        std::string labelName = line.substr(0, colonPos);
        // Trim whitespace
        size_t start = labelName.find_first_not_of(" \t");
        if (start == std::string::npos) return false;
        labelName = labelName.substr(start);

        if (labelName.empty()) return false;
        if (!std::isalpha(labelName[0]) && labelName[0] != '_' && labelName[0] != '.') return false;
        return true;
    }

    // Helper: Parse label name
    std::string parseLabel(const std::string& line) {
        size_t colonPos = line.find(':');
        std::string labelName = line.substr(0, colonPos);
        // Trim whitespace
        size_t start = labelName.find_first_not_of(" \t");
        if (start != std::string::npos) {
            labelName = labelName.substr(start);
        }
        return labelName;
    }

    // Helper: Lookup label in symbol table
    int lookupLabel(const std::string& name) {
        for (const auto& label : symbolTable) {
            if (label.name == name) return label.address;
        }
        return -1;
    }

    // Helper: Parse branch condition
    int parseBranchCondition(const std::string& mnemonic) {
        return findBranchCondition(mnemonic);
    }

    // Encoding functions
    uint32_t encodeAlu(uint8_t op, uint8_t dst, uint16_t src1, uint16_t src2, bool isImmediate) {
        uint32_t instr = 0;
        uint8_t actualOpcode = isImmediate ? (op | 0x10) : op;
        instr |= actualOpcode;
        instr |= ((dst & 0x7) << 8);

        if (!isImmediate) {
            instr |= ((src1 & 0x7) << 12);
            instr |= ((src2 & 0x7) << 16);
        } else {
            instr |= ((src1 & 0x7) << 12);
            instr |= ((src2 & 0xFFFF) << 16);
        }
        return instr;
    }

    uint32_t encodeMove(uint8_t dst, uint16_t srcOrImm, bool isImmediate) {
        uint32_t instr = 0;
        uint8_t moveOp = findOpcode("MOV", isImmediate);
        instr |= moveOp;
        instr |= ((dst & 0x7) << 8);

        if (!isImmediate) {
            instr |= ((srcOrImm & 0x7) << 12);
        } else {
            instr |= ((uint32_t)(srcOrImm & 0xFFFF) << 16);
        }
        return instr;
    }

    uint32_t encodeCmp(uint8_t src1, uint16_t src2, bool isImmediate) {
        uint32_t instr = 0;
        uint8_t cmpOp = findOpcode("CMP", isImmediate);
        instr |= cmpOp;

        if (!isImmediate) {
            instr |= ((src1 & 0x7) << 12);
            instr |= ((src2 & 0x7) << 16);
        } else {
            instr |= ((src1 & 0x7) << 12);
            instr |= ((uint32_t)(src2 & 0xFFFF) << 16);
        }
        return instr;
    }

    uint32_t encodeBranch(uint8_t condition, uint16_t target, bool isImmediate) {
        uint32_t instr = 0;
        uint8_t branchOp = findOpcode("B", isImmediate);
        instr |= branchOp;

        if (isImmediate) {
            // JI format: OPCODE[8] CONDITION[4] [0000] IMMEDIATE[16]
            // Bits 0-7:   Opcode
            // Bits 8-11:  Condition
            // Bits 12-15: Always 0000
            // Bits 16-31: Immediate address (16 bits)
            instr |= ((condition & 0xF) << 8);
            instr |= ((uint32_t)(target & 0xFFFF) << 16);
        } else {
            // J format: OPCODE[8] CONDITION[4] [0000] REG[4] [unused]
            // Bits 0-7:   Opcode
            // Bits 8-11:  Condition
            // Bits 12-15: Always 0000
            // Bits 16-19: Register to jump to
            // Bits 20-31: Unused
            instr |= ((condition & 0xF) << 8);
            instr |= ((target & 0xF) << 16);
        }
        return instr;
    }

    uint32_t encodeRead(uint8_t dst, uint8_t addrReg) {
        uint32_t instr = 0;
        uint8_t readOp = findOpcode("READ", false);
        instr |= readOp;
        instr |= ((dst & 0x7) << 8);
        instr |= ((addrReg & 0x7) << 16);
        return instr;
    }

    uint32_t encodeReadI(uint8_t dst, uint16_t addrImm) {
        uint32_t instr = 0;
        uint8_t readOp = findOpcode("READ", true);
        instr |= readOp;
        instr |= ((dst & 0x7) << 8);
        instr |= ((uint32_t)(addrImm & 0xFFFF) << 16);
        return instr;
    }

    uint32_t encodeWrite(uint8_t dataReg, uint8_t addrReg) {
        uint32_t instr = 0;
        uint8_t writeOp = findOpcode("WRITE", false);
        instr |= writeOp;
        instr |= ((dataReg & 0x7) << 12);
        instr |= ((addrReg & 0x7) << 16);
        return instr;
    }

    uint32_t encodeWriteI(uint8_t dataReg, uint16_t addrImm) {
        uint32_t instr = 0;
        uint8_t writeOp = findOpcode("WRITE", true);
        instr |= writeOp;
        instr |= ((dataReg & 0x7) << 12);
        instr |= ((uint32_t)(addrImm & 0xFFFF) << 16);
        return instr;
    }

    // PRINT encoding per ISA: PRINT <address>, <data>
    // PRINT_REG:    SCN[R[B]] = R[A]  -> address in B (bits 16-18), data in A (bits 12-14)
    // PRINT_REG_I:  SCN[X] = R[A]     -> address in X (bits 16-23), data in A (bits 12-14)
    // PRINT_CONST:  SCN[R[B]] = Y     -> address in B (bits 16-18), data in Y (bits 24-31)
    // PRINT_CONST_I: SCN[X] = Y       -> address in X (bits 16-23), data in Y (bits 24-31)

    uint32_t encodePrintReg(uint8_t dataReg, uint8_t posReg) {
        uint32_t instr = 0;
        uint8_t printOp = findOpcodeByType("PRINT", IsaSpec::InstructionType::TYPE_PRINT_REG, false);
        instr |= printOp;
        instr |= ((dataReg & 0x7) << 12);   // A field: data register
        instr |= ((posReg & 0x7) << 16);    // B field: position register
        return instr;
    }

    uint32_t encodePrintRegI(uint8_t dataReg, uint8_t posImm) {
        uint32_t instr = 0;
        uint8_t printOp = findOpcodeByType("PRINT", IsaSpec::InstructionType::TYPE_PRINT_REG, true);
        instr |= printOp;
        instr |= ((dataReg & 0x7) << 12);          // A field: data register
        instr |= ((uint32_t)(posImm & 0xFF) << 16); // X (lower byte of immediate): position
        return instr;
    }

    uint32_t encodePrintConst(uint8_t dataConst, uint8_t posReg) {
        uint32_t instr = 0;
        uint8_t printOp = findOpcodeByType("PRINT", IsaSpec::InstructionType::TYPE_PRINT_CONST, false);
        instr |= printOp;
        instr |= ((posReg & 0x7) << 16);           // B field: position register
        instr |= ((uint32_t)(dataConst & 0xFF) << 24); // Y (upper byte): data constant
        return instr;
    }

    uint32_t encodePrintConstI(uint16_t dataConst, uint8_t posImm) {
        uint32_t instr = 0;
        uint8_t printOp = findOpcodeByType("PRINT", IsaSpec::InstructionType::TYPE_PRINT_CONST, true);
        instr |= printOp;
        instr |= ((uint32_t)(posImm & 0xFF) << 16);    // X (lower byte): position
        instr |= ((uint32_t)(dataConst & 0xFF) << 24); // Y (upper byte): data constant
        return instr;
    }

    // Helper: Split string into tokens
    std::vector<std::string> tokenize(const std::string& str) {
        std::vector<std::string> tokens;
        std::string current;

        for (char c : str) {
            if (c == ',' || std::isspace(c)) {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        if (!current.empty()) tokens.push_back(current);
        return tokens;
    }

    // Helper: Update ROM data in Digital Logic Sim JSON file
    bool updateDigitalLogicSimRom(const std::vector<uint16_t>& alphaData, const std::vector<uint16_t>& betaData) {
        DigitalLogicSimHelper simHelper("16-CPU");

        std::vector<std::pair<std::string, std::vector<uint16_t>>> updates = {
            {"Machine Code ALPHA", alphaData},
            {"Machine Code BETA", betaData}
        };

        std::cout << "Updating Digital Logic Sim project...\n";
        return simHelper.updateMultipleSubchips(updates);
    }

    // Parse a single instruction line
    uint32_t parseInstruction(const std::string& line, bool& error, int instructionNumber = -1) {
        error = false;

        // Trim leading whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) return 0;
        std::string trimmed = line.substr(start);

        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') return 0;
        if (isLabel(trimmed)) return 0;

        // Extract mnemonic
        size_t spacePos = trimmed.find_first_of(" \t");
        std::string mnemonic = (spacePos == std::string::npos) ? trimmed : trimmed.substr(0, spacePos);

        // Convert to uppercase
        for (char& c : mnemonic) c = std::toupper(c);

        // Extract operands
        std::string operands = (spacePos == std::string::npos) ? "" : trimmed.substr(spacePos + 1);
        std::vector<std::string> tokens = tokenize(operands);

        // LR pseudo-instruction (Load Register with instruction number)
        if (mnemonic == "LR") {
            if (tokens.size() != 1) { error = true; return 0; }
            int dst = parseRegister(tokens[0]);
            if (dst == -1) { error = true; return 0; }
            if (instructionNumber == -1) {
                std::cerr << "Error: LR instruction requires instruction number (internal error)\n";
                error = true;
                return 0;
            }
            // Replace with MOV dst, instructionNumber
            return encodeMove(dst, instructionNumber, true);
        }

        // EXIT instruction
        if (mnemonic == "EXIT") return 0xFFFFFFFF;

        // ALU operations
        if (isAluOperation(mnemonic)) {
            uint8_t op = findOpcode(mnemonic, false);

            if (tokens.size() == 3) {
                int dst = parseRegister(tokens[0]);
                int src1 = parseRegister(tokens[1]);
                if (dst == -1 || src1 == -1) { error = true; return 0; }

                int src2Reg = parseRegister(tokens[2]);
                if (src2Reg != -1) {
                    return encodeAlu(op, dst, src1, src2Reg, false);
                } else {
                    int src2Const = parseConstant(tokens[2]);
                    if (src2Const == -1) { error = true; return 0; }
                    return encodeAlu(op, dst, src1, src2Const, true);
                }
            } else if (tokens.size() == 2) {
                int dst = parseRegister(tokens[0]);
                if (dst == -1) { error = true; return 0; }

                int srcReg = parseRegister(tokens[1]);
                if (srcReg != -1) {
                    return encodeAlu(op, dst, srcReg, 0, false);
                } else {
                    int srcConst = parseConstant(tokens[1]);
                    if (srcConst == -1) { error = true; return 0; }
                    return encodeAlu(op, dst, srcConst, 0, true);
                }
            }
            error = true;
            return 0;
        }

        // NOT operation
        if (mnemonic == "NOT") {
            if (tokens.size() != 1) { error = true; return 0; }
            int dst = parseRegister(tokens[0]);
            if (dst == -1) { error = true; return 0; }
            uint8_t notOp = findOpcode("NOT", false);
            return encodeAlu(notOp, dst, 0, 0, false);
        }

        // MOV operation
        if (mnemonic == "MOV") {
            if (tokens.size() != 2) { error = true; return 0; }
            int dst = parseRegister(tokens[0]);
            if (dst == -1) { error = true; return 0; }

            int srcReg = parseRegister(tokens[1]);
            if (srcReg != -1) {
                return encodeMove(dst, srcReg, false);
            } else {
                int srcConst = parseConstant(tokens[1]);
                if (srcConst == -1) { error = true; return 0; }
                return encodeMove(dst, srcConst, true);
            }
        }

        // CMP operation
        if (mnemonic == "CMP") {
            if (tokens.size() != 2) { error = true; return 0; }
            int src1 = parseRegister(tokens[0]);
            if (src1 == -1) { error = true; return 0; }

            int src2Reg = parseRegister(tokens[1]);
            if (src2Reg != -1) {
                return encodeCmp(src1, src2Reg, false);
            } else {
                int src2Const = parseConstant(tokens[1]);
                if (src2Const == -1) { error = true; return 0; }
                return encodeCmp(src1, src2Const, true);
            }
        }

        // Branch operations
        int condition = parseBranchCondition(mnemonic);
        if (condition >= 0) {
            if (tokens.size() != 1) { error = true; return 0; }

            int targetReg = parseRegister(tokens[0]);
            if (targetReg != -1) {
                return encodeBranch(condition, targetReg, false);
            } else {
                // Try as immediate or label
                int target = parseConstant(tokens[0]);
                if (target == 0 && tokens[0] != "0") {
                    // Try as label
                    target = lookupLabel(tokens[0]);
                    if (target < 0) { error = true; return 0; }
                }
                if (target < 0 || target > 65535) { error = true; return 0; }
                return encodeBranch(condition, target, true);
            }
        }

        // READ operation
        if (mnemonic == "READ") {
            if (tokens.size() != 2) { error = true; return 0; }
            int dst = parseRegister(tokens[0]);
            if (dst == -1) { error = true; return 0; }

            int addrReg = parseRegister(tokens[1]);
            if (addrReg != -1) {
                return encodeRead(dst, addrReg);
            } else {
                int addrImm = parseConstant(tokens[1]);
                if (addrImm < 0 || addrImm > 65535) { error = true; return 0; }
                return encodeReadI(dst, addrImm);
            }
        }

        // WRITE operation
        if (mnemonic == "WRITE") {
            if (tokens.size() != 2) { error = true; return 0; }
            int dataReg = parseRegister(tokens[0]);
            if (dataReg == -1) { error = true; return 0; }

            int addrReg = parseRegister(tokens[1]);
            if (addrReg != -1) {
                return encodeWrite(dataReg, addrReg);
            } else {
                int addrImm = parseConstant(tokens[1]);
                if (addrImm < 0 || addrImm > 65535) { error = true; return 0; }
                return encodeWriteI(dataReg, addrImm);
            }
        }

        // PRINT operation
        if (mnemonic == "PRINT") {
            if (tokens.size() != 2) { error = true; return 0; }

            bool isAddrReg = (parseRegister(tokens[0]) != -1);
            bool isCodeReg = (parseRegister(tokens[1]) != -1);

            if (isAddrReg && isCodeReg) {
                int addrReg = parseRegister(tokens[0]);
                int codeReg = parseRegister(tokens[1]);
                return encodePrintReg(codeReg, addrReg);
            } else if (!isAddrReg && isCodeReg) {
                int addrImm = parseConstant(tokens[0]);
                int codeReg = parseRegister(tokens[1]);
                if (addrImm < 0 || addrImm > 255) { error = true; return 0; }
                return encodePrintRegI(codeReg, addrImm);
            } else if (isAddrReg && !isCodeReg) {
                int addrReg = parseRegister(tokens[0]);
                int codeConst = parseConstant(tokens[1]);
                if (codeConst < 0 || codeConst > 255) { error = true; return 0; }
                return encodePrintConst(codeConst, addrReg);
            } else {
                int addrImm = parseConstant(tokens[0]);
                int codeConst = parseConstant(tokens[1]);
                if (addrImm < 0 || addrImm > 255 || codeConst < 0 || codeConst > 255) { error = true; return 0; }
                return encodePrintConstI(codeConst, addrImm);
            }
        }

        error = true;
        return 0;
    }

public:
    AssemblerTool() : AutoRegisterTool("Assemble Code", "Convert assembly to machine code (ALPHA/BETA ROMs)") {
        // Generate ISA spec algorithmically
        isaSpec = IsaSpec::generateISASpec();
        std::cout << "ISA Specification v" << isaSpec.version << " loaded\n";
        std::cout << "  " << isaSpec.instructions_tech.size() << " technical instructions, "
                  << isaSpec.instructions_doc.size() << " documentation entries, "
                  << isaSpec.branch_conditions.size() << " branch conditions\n";
    }

    void getInputs() override {
        std::cout << "Input assembly file: ";
        std::getline(std::cin, inputFile);

        std::cout << "Output base name (for .out files): ";
        std::getline(std::cin, outputBase);
    }

    void execute(RomFormat outputFormat) override {
        // Open input file
        std::ifstream input(inputFile);
        if (!input.is_open()) {
            std::cerr << "Error: Could not open file '" << inputFile << "'\n";
            return;
        }

        // Pass 1: Build symbol table and alias table
        symbolTable.clear();
        aliasTable.clear();
        std::string line;
        bool inMultiline = false;
        int pc = 0;

        while (std::getline(input, line) && pc < 256) {
            stripComments(line, inMultiline);

            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            std::string trimmed = line.substr(start);
            if (trimmed.empty()) continue;

            // Check for #ALIAS directive
            if (trimmed.length() > 6 && trimmed.substr(0, 6) == "#ALIAS") {
                std::istringstream iss(trimmed.substr(6));
                std::string regName, aliasName;
                iss >> regName >> aliasName;

                // Validate register name (need to temporarily disable alias resolution)
                int regNum = -1;
                if (regName.length() >= 2 && (regName[0] == 'X' || regName[0] == 'x')) {
                    regNum = std::atoi(regName.c_str() + 1);
                }

                if (regNum < 0 || regNum > 7) {
                    std::cerr << "Error: Invalid register in #ALIAS: " << regName << "\n";
                    continue;
                }

                // Validate alias name
                if (!isValidAliasName(aliasName)) {
                    std::cerr << "Error: Invalid alias name: " << aliasName << "\n";
                    std::cerr << "       Alias names must be alphanumeric with underscores only,\n";
                    std::cerr << "       and must not conflict with instruction mnemonics.\n";
                    continue;
                }

                // Add or update alias (subsequent calls overwrite)
                bool found = false;
                for (auto& alias : aliasTable) {
                    if (alias.alias == aliasName) {
                        alias.registerName = regName;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    aliasTable.push_back({aliasName, regName});
                }
                // Don't increment PC for #ALIAS directives
            }
            else if (isLabel(trimmed)) {
                std::string labelName = parseLabel(trimmed);
                symbolTable.push_back({labelName, (uint8_t)pc});
            } else {
                // Regular instruction, increment PC
                pc++;
            }
        }

        // Pass 2: Generate instructions
        input.clear();
        input.seekg(0);
        inMultiline = false;
        std::vector<uint32_t> instructions;

        while (std::getline(input, line) && instructions.size() < 256) {
            stripComments(line, inMultiline);

            bool error = false;
            // Pass current instruction number for LR pseudo-instruction
            uint32_t instr = parseInstruction(line, error, instructions.size());

            if (error) {
                std::cerr << "Warning: Failed to parse line: " << line << "\n";
                continue;
            }

            size_t start = line.find_first_not_of(" \t");
            if (start != std::string::npos) {
                std::string trimmed = line.substr(start);
                // Skip labels and preprocessor directives (like #ALIAS)
                if (!trimmed.empty() && !isLabel(trimmed) && trimmed[0] != '#') {
                    instructions.push_back(instr);
                }
            }
        }
        input.close();

        // Prepare ALPHA and BETA data arrays (256 entries, padded with zeros)
        std::vector<uint16_t> alphaData(256, 0);
        std::vector<uint16_t> betaData(256, 0);

        for (size_t i = 0; i < instructions.size(); i++) {
            alphaData[i] = (instructions[i] >> 16) & 0xFFFF;
            betaData[i] = instructions[i] & 0xFFFF;
        }

        // Write traditional .out ROM files (only if outputBase is not empty)
        if (!outputBase.empty()) {
            RomWriter alphaWriter(outputBase + "_ALPHA.out", outputFormat);
            RomWriter betaWriter(outputBase + "_BETA.out", outputFormat);

            for (size_t i = 0; i < 256; i++) {
                alphaWriter.set(i, alphaData[i]);
                betaWriter.set(i, betaData[i]);
            }

            bool success = true;
            success &= alphaWriter.writeToFile();
            success &= betaWriter.writeToFile();

            if (success) {
                std::cout << "\nCompiled " << instructions.size() << " instructions\n";
                std::cout << "Generated ALPHA ROM: " << outputBase << "_ALPHA.out\n";
                std::cout << "Generated BETA ROM:  " << outputBase << "_BETA.out\n";
            }
        } else {
            std::cout << "\nCompiled " << instructions.size() << " instructions\n";
            std::cout << "Skipping .out file generation (no output base name provided)\n";
        }

        // Update Digital Logic Sim JSON file
        std::cout << "\nUpdating Digital Logic Sim project...\n";
        updateDigitalLogicSimRom(alphaData, betaData);
    }
};

// Opcode Flags ROM Tool
class OpcodeFlagsRomTool : public AutoRegisterTool<OpcodeFlagsRomTool> {
private:
    IsaSpec::ISA_SPEC isaSpec;

public:
    OpcodeFlagsRomTool() : AutoRegisterTool("Opcode Flags ROM", "Generate opcode flags for instruction decoding") {
        isaSpec = IsaSpec::generateISASpec();
    }

    #define FLAG_VALID          (1 << 0)  // Bit 0: Valid instruction
    #define FLAG_TYPE_ALU       (0 << 1)  // Bits 1-4: Instruction type
    #define FLAG_TYPE_FPU       (1 << 1)
    #define FLAG_TYPE_MOVE      (2 << 1)
    #define FLAG_TYPE_CMP       (3 << 1)
    #define FLAG_TYPE_BRANCH    (4 << 1)
    #define FLAG_TYPE_MEMORY    (5 << 1)
    #define FLAG_TYPE_PRINT_REG (6 << 1)
    #define FLAG_TYPE_PRINT_CONST (7 << 1)
    #define FLAG_TYPE_SERVICE   (8 << 1)
    #define FLAG_TYPE_MASK      (15 << 1)
    #define FLAG_IMMEDIATE      (1 << 5)  // Bit 5: Is immediate format
    #define FLAG_OVERRIDE_WRITE (1 << 11) // Bit 11: OVERRIDE WRITE flag
    #define FLAG_OVERRIDE_B     (1 << 12) // Bit 12: OVERRIDE B flag
    #define FLAG_TRY_READ_A     (1 << 13) // Bit 13: Try read A operand
    #define FLAG_TRY_READ_B     (1 << 14) // Bit 14: Try read B operand
    #define FLAG_TRY_WRITE      (1 << 15) // Bit 15: Try write result

    uint16_t encodeInstructionFlags(const IsaSpec::InstructionTech& instr) {
        uint16_t flags = 0;

        // Valid flag
        if (instr.flags.VALID) flags |= FLAG_VALID;

        // Instruction type
        switch (instr.type) {
            case IsaSpec::InstructionType::TYPE_ALU:         flags |= FLAG_TYPE_ALU; break;
            case IsaSpec::InstructionType::TYPE_FPU:         flags |= FLAG_TYPE_FPU; break;
            case IsaSpec::InstructionType::TYPE_MOVE:        flags |= FLAG_TYPE_MOVE; break;
            case IsaSpec::InstructionType::TYPE_CMP:         flags |= FLAG_TYPE_CMP; break;
            case IsaSpec::InstructionType::TYPE_BRANCH:      flags |= FLAG_TYPE_BRANCH; break;
            case IsaSpec::InstructionType::TYPE_MEMORY:      flags |= FLAG_TYPE_MEMORY; break;
            case IsaSpec::InstructionType::TYPE_PRINT_REG:   flags |= FLAG_TYPE_PRINT_REG; break;
            case IsaSpec::InstructionType::TYPE_PRINT_CONST: flags |= FLAG_TYPE_PRINT_CONST; break;
            case IsaSpec::InstructionType::TYPE_SERVICE:     flags |= FLAG_TYPE_SERVICE; break;
        }

        // Other flags
        if (instr.flags.IMMEDIATE) flags |= FLAG_IMMEDIATE;
        if (instr.flags.OVERRIDE_WRITE) flags |= FLAG_OVERRIDE_WRITE;
        if (instr.flags.OVERRIDE_B) flags |= FLAG_OVERRIDE_B;
        if (instr.flags.TRY_READ_A) flags |= FLAG_TRY_READ_A;
        if (instr.flags.TRY_READ_B) flags |= FLAG_TRY_READ_B;
        if (instr.flags.TRY_WRITE) flags |= FLAG_TRY_WRITE;

        return flags;
    }

    void execute(RomFormat outputFormat) override {
        RomWriter writer("rom_out/OPCODE_FLAGS.out", outputFormat);
        std::vector<uint16_t> flagsData(256, 0);

        // Generate flags from ISA spec
        for (const auto& instr : isaSpec.instructions_tech) {
            uint16_t flags = encodeInstructionFlags(instr);
            writer.set(instr.opcode, flags);
            flagsData[instr.opcode] = flags;
        }

        // Write to file
        if (writer.writeToFile()) {
            std::cout << "Successfully generated OPCODE_FLAGS ROM from ISA spec\n";
            std::cout << "  " << isaSpec.instructions_tech.size() << " instructions encoded\n";
        }

        // Update Digital Logic Sim JSON file
        DigitalLogicSimHelper simHelper("Machine code parser");
        std::cout << "Updating Digital Logic Sim project...\n";
        simHelper.updateSubchipData("OP CODE PARSER", flagsData);
    }
};

// Branch Condition ROM Tool
class BranchConditionRomTool : public AutoRegisterTool<BranchConditionRomTool> {
public:
    BranchConditionRomTool() : AutoRegisterTool("Branch Condition ROM", "Generate branch condition lookup table") {}

    void execute(RomFormat outputFormat) override {
        RomWriter writer("rom_out/BRANCH_CONDITIONS_LUT.out", outputFormat);

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
            writer.set((uint8_t)addr, should_branch ? 0xFFFF : 0x0000);
        }

        std::cout << "Would generate: BRANCH_CONDITION\n";
    }
};

// Instruction Type Display ROM Tool
class InstructionTypeDisplayRomTool : public AutoRegisterTool<InstructionTypeDisplayRomTool> {
private:
    IsaSpec::ISA_SPEC isaSpec;

public:
    InstructionTypeDisplayRomTool() : AutoRegisterTool("Instruction Type Display ROM", "Generate instruction type name lookup table") {
        isaSpec = IsaSpec::generateISASpec();
    }

    void execute(RomFormat outputFormat) override {
        RomWriter writerCharlie("rom_out/INSTRUCTION_TYPE_DISPLAY_CHARLIE.out", outputFormat);
        RomWriter writerBeta("rom_out/INSTRUCTION_TYPE_DISPLAY_BETA.out", outputFormat);
        RomWriter writerAlpha("rom_out/INSTRUCTION_TYPE_DISPLAY_ALPHA.out", outputFormat);

        std::vector<uint16_t> charlieData(256, 0);
        std::vector<uint16_t> betaData(256, 0);
        std::vector<uint16_t> alphaData(256, 0);

        // Encode instruction technical names into 9 characters (5 bits each)
        // Total: 45 bits across 3 ROMs (48 bits total, 3 garbage bits)
        // Encoding: a=00000, b=00001, ..., z=11001, blank=11111
        // Layout (LSB-first):
        //   CHARLIE bits 4-0:   char 0 (first letter)
        //   CHARLIE bits 9-5:   char 1
        //   CHARLIE bits 14-10: char 2
        //   CHARLIE bit 15 + BETA bits 3-0: char 3
        //   BETA bits 8-4:   char 4
        //   BETA bits 13-9:  char 5
        //   BETA bits 15-14 + ALPHA bits 2-0: char 6
        //   ALPHA bits 7-3:  char 7
        //   ALPHA bits 12-8: char 8
        //   ALPHA bits 15-13: garbage (3 bits)
        // Underscores are skipped, convert to lowercase

        // Generate entry for each instruction in the ISA spec
        for (const auto& instr : isaSpec.instructions_tech) {
            uint64_t encoded = 0;
            int charCount = 0;

            // Convert technical name to lowercase, skip underscores, take first 9 chars
            std::string name = instr.technical_name;
            for (size_t i = 0; i < name.length() && charCount < 9; i++) {
                char c = name[i];

                // Skip underscores
                if (c == '_') continue;

                // Convert to lowercase
                if (c >= 'A' && c <= 'Z') {
                    c = c - 'A' + 'a';
                }

                // Encode if it's a letter
                if (c >= 'a' && c <= 'z') {
                    uint8_t charValue = c - 'a';
                    // Pack into position (LSB first)
                    encoded |= ((uint64_t)charValue << (charCount * 5));
                    charCount++;
                }
            }

            // Pad remaining characters with 0b11111 (blank) if less than 9
            while (charCount < 9) {
                encoded |= ((uint64_t)0x1F << (charCount * 5));
                charCount++;
            }

            // Split into three ROMs: CHARLIE (bits 15-0), BETA (bits 31-16), ALPHA (bits 47-32)
            uint16_t valueCharlie = encoded & 0xFFFF;
            uint16_t valueBeta = (encoded >> 16) & 0xFFFF;
            uint16_t valueAlpha = (encoded >> 32) & 0xFFFF;

            writerCharlie.set(instr.opcode, valueCharlie);
            writerBeta.set(instr.opcode, valueBeta);
            writerAlpha.set(instr.opcode, valueAlpha);

            charlieData[instr.opcode] = valueCharlie;
            betaData[instr.opcode] = valueBeta;
            alphaData[instr.opcode] = valueAlpha;
        }

        bool success = true;
        success &= writerCharlie.writeToFile();
        success &= writerBeta.writeToFile();
        success &= writerAlpha.writeToFile();

        if (success) {
            std::cout << "Successfully generated INSTRUCTION_TYPE_DISPLAY ROMs\n";
            std::cout << "  CHARLIE: INSTRUCTION_TYPE_DISPLAY_CHARLIE.out\n";
            std::cout << "  BETA: INSTRUCTION_TYPE_DISPLAY_BETA.out\n";
            std::cout << "  ALPHA: INSTRUCTION_TYPE_DISPLAY_ALPHA.out\n";
            std::cout << "  " << isaSpec.instructions_tech.size() << " instructions encoded (9 chars each, 5 bits per char)\n";
        }

        // Update Digital Logic Sim JSON file
        DigitalLogicSimHelper simHelper("OP CODE DISPLAY DRIVER");

        std::vector<std::pair<std::string, std::vector<uint16_t>>> updates = {
            {"CHARLIE", charlieData},
            {"BETA", betaData},
            {"ALPHA", alphaData}
        };

        std::cout << "Updating Digital Logic Sim project...\n";
        simHelper.updateMultipleSubchips(updates);
    }
};

// Hex Display ROM Tool
class HexDisplayRomTool : public AutoRegisterTool<HexDisplayRomTool> {
private:
    // Convert a 4-bit nibble (0-15) to ASCII hex digit (uppercase: 0-9, A-F)
    uint8_t nibbleToAsciiUpper(uint8_t nibble) {
        if (nibble < 10) {
            return '0' + nibble;
        } else {
            return 'A' + (nibble - 10);
        }
    }

    // Convert a 4-bit nibble (0-15) to ASCII hex digit (lowercase: 0-9, a-f)
    uint8_t nibbleToAsciiLower(uint8_t nibble) {
        if (nibble < 10) {
            return '0' + nibble;
        } else {
            return 'a' + (nibble - 10);
        }
    }

public:
    HexDisplayRomTool() : AutoRegisterTool("Hex Display ROM", "Generate hex-to-ASCII display ROMs") {}

    void execute(RomFormat outputFormat) override {
        // ROM 1: HEX_4_ASCII (4-bit input -> ASCII hex digit)
        RomWriter hex4Writer("rom_out/HEX_4_ASCII.out", outputFormat);
        for (int i = 0; i < 256; i++) {
            if (i < 16) {
                hex4Writer.set(i, nibbleToAsciiUpper(i));
            } else {
                hex4Writer.set(i, 0x00);
            }
        }

        // ROM 2: HEX_8_ASCII_LOWER (8-bit input -> 16-bit output, lowercase)
        RomWriter hex8LowerWriter("rom_out/HEX_8_ASCII_LOWER.out", outputFormat);
        for (int addr = 0; addr < 256; addr++) {
            uint8_t lowerNibble = addr & 0xF;
            uint8_t upperNibble = (addr >> 4) & 0xF;
            uint16_t result = ((uint16_t)nibbleToAsciiLower(upperNibble) << 8) | nibbleToAsciiLower(lowerNibble);
            hex8LowerWriter.set(addr, result);
        }

        // ROM 3: HEX_8_ASCII_UPPER (8-bit input -> 16-bit output, uppercase)
        RomWriter hex8UpperWriter("rom_out/HEX_8_ASCII_UPPER.out", outputFormat);
        for (int addr = 0; addr < 256; addr++) {
            uint8_t lowerNibble = addr & 0xF;
            uint8_t upperNibble = (addr >> 4) & 0xF;
            uint16_t result = ((uint16_t)nibbleToAsciiUpper(upperNibble) << 8) | nibbleToAsciiUpper(lowerNibble);
            hex8UpperWriter.set(addr, result);
        }

        // Write all three ROMs
        bool success = true;
        success &= hex4Writer.writeToFile();
        success &= hex8LowerWriter.writeToFile();
        success &= hex8UpperWriter.writeToFile();

        if (success) {
            std::cout << "\nSuccessfully generated hex display ROMs:\n";
            std::cout << "  HEX_4_ASCII.out (4-bit -> ASCII, uppercase)\n";
            std::cout << "  HEX_8_ASCII_LOWER.out (8-bit -> 16-bit, lowercase)\n";
            std::cout << "  HEX_8_ASCII_UPPER.out (8-bit -> 16-bit, uppercase)\n";
        }
    }
};

// ASCII Font ROM Tool
class AsciiFontRomTool : public AutoRegisterTool<AsciiFontRomTool> {
private:
    std::string bmpFile;

public:
    AsciiFontRomTool() : AutoRegisterTool("ASCII Font ROM", "Generate font ROMs from 8x8 BMP atlas") {}

    void getInputs() override {
        std::cout << "BMP font file: ";
        std::getline(std::cin, bmpFile);
    }

    void execute(RomFormat outputFormat) override {
        std::cout << "\n[TODO: Implement ASCII font ROM generator]\n";
        std::cout << "Would process: " << bmpFile << "\n";
        std::cout << "Would generate: ROM_ALPHA/BRAVO/CHARLIE/DELTA\n";
    }
};

// FP16 Digit Masks ROM Tool
class Fp16DigitMasksRomTool : public AutoRegisterTool<Fp16DigitMasksRomTool> {
private:
    enum FPCODE {
        FPC_ZERO = 0x8,
        FPC_NUM  = 0x4,
        FPC_INF  = 0x2,
        FPC_NAN  = 0x1
    };

public:
    Fp16DigitMasksRomTool() : AutoRegisterTool("FP16 Digit Masks ROM", "Generate FP16 digit mask lookup table") {}

    void execute(RomFormat outputFormat) override {
        RomWriter writer("rom_out/fp16_digitmask", outputFormat);

        const FPCODE fpCodes[] = {FPC_ZERO, FPC_NUM, FPC_INF, FPC_NAN};

        for (int i = 0; i < 4; i++) {
            FPCODE code = fpCodes[i];

            for (uint8_t cell = 0; cell <= 9; cell++) {
                // Encode address: [fp_code:cell_idx]
                uint8_t addr = ((uint8_t)code << 4) | cell;
                uint8_t character = 0;

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

                writer.set(addr, (character << 8) | character);
            }
        }

        if (writer.writeToFile()) {
            std::cout << "Successfully generated FP16 digit mask ROM\n";
        }
    }
};

// ISA Documentation Generator Tool
class IsaDocGeneratorTool : public AutoRegisterTool<IsaDocGeneratorTool> {
private:
    IsaSpec::ISA_SPEC isaSpec;

public:
    IsaDocGeneratorTool() : AutoRegisterTool("ISA Documentation Generator", "Update isa.md from IsaSpec.hpp") {
        isaSpec = IsaSpec::generateISASpec();
    }

    void getInputs() override {
        // No inputs needed
    }

    void execute(RomFormat outputFormat) override {
        std::cout << "\n--- ISA Documentation Generator ---\n";
        std::cout << "Generating isa.md from IsaSpec.hpp...\n\n";

        // Group instructions by type for documentation
        std::map<IsaSpec::InstructionType, std::vector<std::pair<const IsaSpec::InstructionTech*, const IsaSpec::InstructionDoc*>>> groupedInstructions;

        for (const auto& tech : isaSpec.instructions_tech) {
            // Find corresponding documentation
            const IsaSpec::InstructionDoc* doc = nullptr;
            for (const auto& d : isaSpec.instructions_doc) {
                if (d.technical_name == tech.technical_name) {
                    doc = &d;
                    break;
                }
            }
            groupedInstructions[tech.type].push_back({&tech, doc});
        }

        // Generate markdown file
        std::ofstream md("isa.md");
        if (!md.is_open()) {
            std::cerr << "Error: Could not open isa.md for writing\n";
            return;
        }

        // Write header
        md << "# V2 ISA Specification\n\n";
        md << "A 16-bit instruction set architecture for a custom computer built in Digital Logic Sim.\n\n";
        md << "## Overview\n\n";
        md << "Instructions are **32-bit** (4 bytes), with the first byte always serving as the opcode.\n\n";
        md << "```\n";
        md << "Byte Layout: OPCODE[xxxx xxxx] [parameter bytes 1-3]\n";
        md << "```\n\n";
        md << "**Architecture:**\n";
        md << "- **Registers:** " << isaSpec.architecture.register_count << " general-purpose registers (X0-X"
           << (isaSpec.architecture.register_count - 1) << "), each " << isaSpec.architecture.register_width << "-bit\n";
        md << "- **Memory:** " << isaSpec.architecture.memory_size << "x" << isaSpec.architecture.memory_width
           << " RAM unit (" << isaSpec.architecture.memory_size << " addresses, " << isaSpec.architecture.memory_width << "-bit words)\n\n";

        // Instruction formats
        md << "## Instruction Formats\n\n";
        md << "### Register Format (R)\n";
        md << "```\n";
        md << "Bit Layout: OPCODE[8] DST[3]0 A[3]0 B[3]0 [unused]\n";
        md << "```\n";
        md << "Register-to-register operations with 1 destination and 2 source registers.\n\n";

        md << "### Immediate Format (I)\n";
        md << "```\n";
        md << "Bit Layout: OPCODE[8] DST[3]0 A[3]0 IMMEDIATE[16]\n";
        md << "```\n";
        md << "Register and immediate operations with a 16-bit immediate value.\n\n";

        md << "### Branch Register Format (J)\n";
        md << "```\n";
        md << "Bit Layout: OPCODE[8] CONDITION[4] [0000] REG[4] [unused 12 bits]\n";
        md << "            Bits 0-7  8------11  12--15 16-19  20-----------31\n";
        md << "```\n";
        md << "Branch to address in register REG if CONDITION is met.\n";
        md << "- CONDITION: 4-bit condition code (see branch conditions table)\n";
        md << "- Bits 12-15 are always 0000\n";
        md << "- REG: 4-bit register number (0-7, only X0-X7 valid)\n\n";

        md << "### Branch Immediate Format (JI)\n";
        md << "```\n";
        md << "Bit Layout: OPCODE[8] CONDITION[4] [0000] IMMEDIATE[16]\n";
        md << "            Bits 0-7  8------11  12--15 16-------------31\n";
        md << "```\n";
        md << "Branch to immediate address if CONDITION is met.\n";
        md << "- CONDITION: 4-bit condition code (see branch conditions table)\n";
        md << "- Bits 12-15 are always 0000\n";
        md << "- IMMEDIATE: 16-bit immediate address (0-65535)\n\n\n";
        md << "---\n\n";

        // Operations
        md << "## Operations\n\n";

        // ALU Operations
        auto aluInstructions = groupedInstructions[IsaSpec::InstructionType::TYPE_ALU];
        if (!aluInstructions.empty()) {
            md << "<details open>\n";
            md << "<summary><b>ALU Operations</b></summary>\n\n";

            // Split into R and I formats
            std::vector<std::pair<const IsaSpec::InstructionTech*, const IsaSpec::InstructionDoc*>> regFormat, immFormat;
            for (const auto& pair : aluInstructions) {
                if (pair.first->format == IsaSpec::Format::R) {
                    regFormat.push_back(pair);
                } else {
                    immFormat.push_back(pair);
                }
            }

            // Register format table
            md << "### Register Format [0x00-0x0F]\n\n";
            md << "| OPCODE | Instruction | Format | Description | Usage Example | Behaviour |\n";
            md << "|--------|-------------|--------|-------------|---------------|-----------||\n";
            for (const auto& pair : regFormat) {
                const auto* tech = pair.first;
                const auto* doc = pair.second;
                md << "| 0x" << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (int)tech->opcode << std::dec << " | ";
                md << tech->technical_name << " | R | ";
                if (doc) {
                    md << "`" << doc->description << "` | `" << doc->usage_example << "` | " << doc->explanation;
                } else {
                    md << "- | - | -";
                }
                md << " |\n";
            }

            // Immediate format table
            md << "\n### Immediate Format [0x10-0x1F]\n\n";
            md << "| OPCODE | Instruction | Format | Description | Usage Example | Behaviour |\n";
            md << "|--------|-------------|--------|-------------|---------------|-----------||\n";
            for (const auto& pair : immFormat) {
                const auto* tech = pair.first;
                const auto* doc = pair.second;
                md << "| 0x" << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (int)tech->opcode << std::dec << " | ";
                md << tech->technical_name << " | I | ";
                if (doc) {
                    md << "`" << doc->description << "` | `" << doc->usage_example << "` | " << doc->explanation;
                } else {
                    md << "- | - | -";
                }
                md << " |\n";
            }

            md << "\n</details>\n\n";
        }

        // MOV Operations
        auto moveInstructions = groupedInstructions[IsaSpec::InstructionType::TYPE_MOVE];
        if (!moveInstructions.empty()) {
            md << "<details open>\n";
            md << "<summary><b>MOV Operations</b></summary>\n\n";
            md << "| OPCODE | Instruction | Format | Description | Usage Example | Behaviour |\n";
            md << "|--------|-------------|--------|-------------|---------------|-----------||\n";
            for (const auto& pair : moveInstructions) {
                const auto* tech = pair.first;
                const auto* doc = pair.second;
                md << "| 0x" << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (int)tech->opcode << std::dec << " | ";
                md << tech->technical_name << " | " << (tech->format == IsaSpec::Format::R ? "R" : "I") << " | ";
                if (doc) {
                    md << "`" << doc->description << "` | `" << doc->usage_example << "` | " << doc->explanation;
                } else {
                    md << "- | - | -";
                }
                md << " |\n";
            }
            md << "\n</details>\n\n\n";
        }

        // Control Flow Operations (CMP + BRANCH)
        auto cmpInstructions = groupedInstructions[IsaSpec::InstructionType::TYPE_CMP];
        auto branchInstructions = groupedInstructions[IsaSpec::InstructionType::TYPE_BRANCH];
        if (!cmpInstructions.empty() || !branchInstructions.empty()) {
            md << "<details open>\n";
            md << "<summary><b>Control Flow Operations</b></summary>\n\n";

            if (!cmpInstructions.empty()) {
                md << "### Comparison Operations\n\n";
                md << "| OPCODE | Instruction | Format | Description | Usage Example | Behaviour |\n";
                md << "|--------|-------------|--------|-------------|---------------|-----------||\n";
                for (const auto& pair : cmpInstructions) {
                    const auto* tech = pair.first;
                    const auto* doc = pair.second;
                    md << "| 0x" << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (int)tech->opcode << std::dec << " | ";
                    md << tech->technical_name << " | " << (tech->format == IsaSpec::Format::R ? "R" : "I") << " | ";
                    if (doc) {
                        md << "`" << doc->description << "` | `" << doc->usage_example << "` | " << doc->explanation;
                    } else {
                        md << "- | - | -";
                    }
                    md << " |\n";
                }
                md << "\n";
            }

            if (!branchInstructions.empty()) {
                md << "### Branch Operations\n\n";
                md << "| OPCODE | Instruction | Format | Description | Usage Example | Behaviour |\n";
                md << "|--------|-------------|--------|-------------|---------------|-----------||\n";
                for (const auto& pair : branchInstructions) {
                    const auto* tech = pair.first;
                    const auto* doc = pair.second;
                    md << "| 0x" << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (int)tech->opcode << std::dec << " | ";
                    md << tech->technical_name << " | " << (tech->format == IsaSpec::Format::J ? "J" : "JI") << " | ";
                    if (doc) {
                        md << "`" << doc->description << "` | `" << doc->usage_example << "` | " << doc->explanation;
                    } else {
                        md << "- | - | -";
                    }
                    md << " |\n";
                }
            }

            md << "\n</details>\n\n\n";
        }

        // Memory Operations
        auto memInstructions = groupedInstructions[IsaSpec::InstructionType::TYPE_MEMORY];
        if (!memInstructions.empty()) {
            md << "<details open>\n";
            md << "<summary><b>Memory  </b></summary>\n\n";
            md << "| OPCODE | Instruction | Format | Description | Usage Example | Behaviour |\n";
            md << "|--------|-------------|--------|-------------|---------------|-----------||\n";
            for (const auto& pair : memInstructions) {
                const auto* tech = pair.first;
                const auto* doc = pair.second;
                md << "| 0x" << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (int)tech->opcode << std::dec << " | ";
                md << tech->technical_name << " | " << (tech->format == IsaSpec::Format::R ? "R" : "I") << " | ";
                if (doc) {
                    md << "`" << doc->description << "` | `" << doc->usage_example << "` | " << doc->explanation;
                } else {
                    md << "- | - | -";
                }
                md << " |\n";
            }
            md << "\n</details>\n\n";
        }

        // Print Operations
        auto printRegInstructions = groupedInstructions[IsaSpec::InstructionType::TYPE_PRINT_REG];
        auto printConstInstructions = groupedInstructions[IsaSpec::InstructionType::TYPE_PRINT_CONST];
        if (!printRegInstructions.empty() || !printConstInstructions.empty()) {
            md << "<details open>\n";
            md << "<summary><b>Printing  </b></summary>\n\n";
            md << "In reference to Print instructions of Format `I`, the symbol `Y` refers to the most significant byte of `IMMEDIATE`. ";
            md << "Similarly, the symbol `X` refers to least significant byte of `IMMEDIATE`\n\n";
            md << "| OPCODE | Instruction | Format | Description | Usage Example | Behaviour |\n";
            md << "|--------|-------------|--------|-------------|---------------|-----------||\n";

            // Combine and sort print instructions
            std::vector<std::pair<const IsaSpec::InstructionTech*, const IsaSpec::InstructionDoc*>> allPrintInstructions;
            allPrintInstructions.insert(allPrintInstructions.end(), printRegInstructions.begin(), printRegInstructions.end());
            allPrintInstructions.insert(allPrintInstructions.end(), printConstInstructions.begin(), printConstInstructions.end());
            std::sort(allPrintInstructions.begin(), allPrintInstructions.end(),
                     [](const auto& a, const auto& b) { return a.first->opcode < b.first->opcode; });

            for (const auto& pair : allPrintInstructions) {
                const auto* tech = pair.first;
                const auto* doc = pair.second;
                md << "| 0x" << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (int)tech->opcode << std::dec << " | ";
                md << tech->technical_name << " | " << (tech->format == IsaSpec::Format::R ? "R" : "I") << " | ";
                if (doc) {
                    md << "`" << doc->description << "` | `" << doc->usage_example << "` | " << doc->explanation;
                } else {
                    md << "- | - | -";
                }
                md << " |\n";
            }
            md << "\n</details>\n\n\n\n";
        }

        // Branch conditions
        md << "\n## Branching (Detailed)\n\n";
        md << "### Branch Conditions\n\n";
        md << "| Code | Mnemonic | Name | Description |\n";
        md << "|------|----------|------|-------------|\n";
        for (const auto& bc : isaSpec.branch_conditions) {
            md << "| 0x" << std::hex << std::setw(1) << std::uppercase << (int)bc.code << std::dec << " | ";
            md << bc.mnemonic << " | " << bc.name << " | " << bc.description << " |\n";
        }

        md << "\n---\n\n";

        // Register file
        md << "## Register File\n\n";
        md << "- **Count:** " << isaSpec.architecture.register_count << " general-purpose registers (0-"
           << (isaSpec.architecture.register_count - 1) << ")\n";
        md << "- **Width:** " << isaSpec.architecture.register_width << "-bit\n\n";
        md << "---\n\n";

        // Machine Code Translator ROM
        md << "## Machine Code Translator ROM\n\n";
        md << "The Machine Code Translator ROM is a 256-entry lookup table that describes instruction properties based on opcode. ";
        md << "The address equals the opcode (0x00-0xFF), and the 16-bit value contains flags describing instruction characteristics.\n\n";
        md << "### Bit Flags\n\n";
        md << "| Bit | Name | Description |\n";
        md << "|-----|------|-------------|\n";
        md << "| 15 | TRY_WRITE | Try write result |\n";
        md << "| 14 | TRY_READ_B | Try read B operand |\n";
        md << "| 13 | TRY_READ_A | Try read A operand |\n";
        md << "| 12 | OVERRIDE_B | OVERRIDE B flag (for ALU_I commands) |\n";
        md << "| 11 | OVERRIDE_WRITE | OVERRIDE WRITE flag (for MOV commands) |\n";
        md << "| 5 | IMMEDIATE | Is immediate format variant |\n";
        md << "| **1-4** | **TYPE** | **Instruction type (bits 1-4)** |\n";
        md << "| | TYPE_ALU (0) | ALU Operations |\n";
        md << "| | TYPE_FPU (1) | FPU Operations (reserved) |\n";
        md << "| | TYPE_MOVE (2) | Move Operations |\n";
        md << "| | TYPE_CMP (3) | Comparison Operations |\n";
        md << "| | TYPE_BRANCH (4) | Branch/Jump Operations |\n";
        md << "| | TYPE_MEMORY (5) | Memory Operations (constant or register addressing) |\n";
        md << "| | TYPE_PRINT_REG (6) | PRINT register data (position determined by IMMEDIATE flag) |\n";
        md << "| | TYPE_PRINT_CONST (7) | PRINT constant data (position determined by IMMEDIATE flag) |\n";
        md << "| | TYPE_SERVICE (8) | Service/System Operations |\n";
        md << "| 0 | VALID | Instruction is valid |\n\n";
        md << "---\n\n";

        // Footer
        md << "## Notes\n\n";
        md << "- The ISA is currently in development\n";
        md << "- Opcode assignments may change during design phase\n";

        md.close();

        std::cout << "Successfully generated isa.md\n";
        std::cout << "  " << isaSpec.instructions_tech.size() << " instructions documented\n";
        std::cout << "  " << isaSpec.branch_conditions.size() << " branch conditions listed\n";
    }
};

// ============================================
// TOOL REGISTRATION - Add your tools here!
// ============================================
// Simple macro to register a tool
#define REGISTER_TOOL(ToolClass) ToolRegistry::registerTool(new ToolClass())

void registerAllTools() {
    REGISTER_TOOL(AssemblerTool);
    REGISTER_TOOL(OpcodeFlagsRomTool);
    REGISTER_TOOL(BranchConditionRomTool);
    REGISTER_TOOL(InstructionTypeDisplayRomTool);
    REGISTER_TOOL(HexDisplayRomTool);
    REGISTER_TOOL(AsciiFontRomTool);
    REGISTER_TOOL(Fp16DigitMasksRomTool);
    REGISTER_TOOL(IsaDocGeneratorTool);
    // Add new tools here with: REGISTER_TOOL(YourNewTool);
}

// ============================================
// MAIN TOOLSET MANAGER
// ============================================

class GateComputerToolset {
private:
    RomFormat outputFormat;

    void clearScreen() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }

    std::string formatToString(RomFormat fmt) const {
        switch (fmt) {
            case ROM_HEX: return "hex";
            case ROM_UINT: return "uint";
            case ROM_INT: return "int";
            case ROM_BINARY: return "binary";
            default: return "unknown";
        }
    }

    void printHeader() {
        std::cout << "========================================\n";
        std::cout << "  Gate Computer Toolset v2.0\n";
        std::cout << "  Assembly & ROM Generation\n";
        std::cout << "========================================\n";
        std::cout << "Output Format: " << formatToString(outputFormat) << "\n\n";
    }

    void printMenu() {
        const auto& tools = ToolRegistry::getAllTools();
        std::cout << "Main Menu:\n";

        // Dynamically print tools from the registry
        for (size_t i = 0; i < tools.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << tools[i]->name << "\n";
        }

        std::cout << "  " << (tools.size() + 1) << ". Settings\n";
        std::cout << "  0. Exit\n\n";
    }

    int getMenuChoice() {
        const auto& tools = ToolRegistry::getAllTools();
        int maxChoice = tools.size() + 1;
        int choice;

        while (true) {
            std::cout << "Enter choice: ";

            if (std::cin >> choice) {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                if (choice >= 0 && choice <= maxChoice) {
                    return choice;
                }
                std::cout << "Invalid choice. Please enter 0-" << maxChoice << ".\n";
            } else {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid input. Please enter a number.\n";
            }
        }
    }

    void handleMenuChoice(int choice) {
        if (choice == 0) {
            return; // Exit
        }

        const auto& tools = ToolRegistry::getAllTools();
        int settingsIndex = tools.size() + 1;

        if (choice == settingsIndex) {
            showSettings();
        } else if (choice >= 1 && choice <= (int)tools.size()) {
            runTool(choice - 1);
        }
    }

    void runTool(int index) {
        const auto& tools = ToolRegistry::getAllTools();
        Tool* tool = tools[index];

        std::cout << "\n--- " << tool->name << " ---\n";
        std::cout << tool->description << "\n\n";

        // Get inputs from user
        tool->getInputs();

        // Execute the tool
        tool->execute(outputFormat);

        pressEnterToContinue();
    }

    void showSettings() {
        std::cout << "\n--- Settings ---\n";
        std::cout << "Current output format: " << formatToString(outputFormat) << "\n\n";

        std::cout << "Select output format:\n";
        std::cout << "  1. Hex\n";
        std::cout << "  2. Unsigned Int\n";
        std::cout << "  3. Signed Int\n";
        std::cout << "  4. Binary\n";
        std::cout << "  0. Back to main menu\n\n";
        std::cout << "Choice: ";

        int choice;
        if (std::cin >> choice) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            switch (choice) {
                case 1:
                    outputFormat = ROM_HEX;
                    std::cout << "\nOutput format set to: hex\n";
                    break;
                case 2:
                    outputFormat = ROM_UINT;
                    std::cout << "\nOutput format set to: uint\n";
                    break;
                case 3:
                    outputFormat = ROM_INT;
                    std::cout << "\nOutput format set to: int\n";
                    break;
                case 4:
                    outputFormat = ROM_BINARY;
                    std::cout << "\nOutput format set to: binary\n";
                    break;
                case 0:
                    return;
                default:
                    std::cout << "\nInvalid choice. Format unchanged.\n";
            }
        } else {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "\nInvalid input. Format unchanged.\n";
        }

        pressEnterToContinue();
    }

    void pressEnterToContinue() {
        std::cout << "\nPress Enter to continue...";
        std::cin.get();
    }

public:
    GateComputerToolset() : outputFormat(ROM_HEX) {
        // No need to manually register tools - they register themselves!
    }

    void run() {
        while (true) {
            clearScreen();
            printHeader();
            printMenu();

            int choice = getMenuChoice();

            if (choice == 0) {
                std::cout << "\nExiting Gate Computer Toolset. Goodbye!\n";
                break;
            }

            handleMenuChoice(choice);
        }
    }
};

// ISA Documentation Generator Tool

int main() {
    // Register all tools first
    registerAllTools();

    // Run the toolset
    GateComputerToolset toolset;
    toolset.run();
    return 0;
}
