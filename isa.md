# V2 ISA Specification

A 16-bit instruction set architecture for a custom computer built in Digital Logic Sim.

## Overview

Instructions are **32-bit** (4 bytes), with the first byte always serving as the opcode.

```
Byte Layout: OPCODE[xxxx xxxx] [parameter bytes 1-3]
```

**Architecture:**
- **Registers:** 8 general-purpose registers (X0-X7), each 16-bit
- **Memory:** 256x16 RAM unit (256 addresses, 16-bit words)

## Instruction Formats

### Register Format (R)
```
Bit Layout: OPCODE[8] DST[3]0 A[3]0 B[3]0 [unused]
```
Register-to-register operations with 1 destination and 2 source registers.

### Immediate Format (I)
```
Bit Layout: OPCODE[8] DST[3]0 A[3]0 IMMEDIATE[16]
```
Register and immediate operations with a 16-bit immediate value.

### Branch Register Format (J)
```
Bit Layout: OPCODE[8] CONDITION[4] [0000] REG[4] [unused 12 bits]
            Bits 0-7  8------11  12--15 16-19  20-----------31
```
Branch to address in register REG if CONDITION is met.
- CONDITION: 4-bit condition code (see branch conditions table)
- Bits 12-15 are always 0000
- REG: 4-bit register number (0-7, only X0-X7 valid)

### Branch Immediate Format (JI)
```
Bit Layout: OPCODE[8] CONDITION[4] [0000] IMMEDIATE[16]
            Bits 0-7  8------11  12--15 16-------------31
```
Branch to immediate address if CONDITION is met.
- CONDITION: 4-bit condition code (see branch conditions table)
- Bits 12-15 are always 0000
- IMMEDIATE: 16-bit immediate address (0-65535)


---

## Operations

<details open>
<summary><b>ALU Operations</b></summary>

### Register Format [0x00-0x0F]

| OPCODE | Instruction | Format | Description | Usage Example | Behaviour |
|--------|-------------|--------|-------------|---------------|-----------||
| 0x00 | ALU_AND | R | `R[DST] = R[A] & R[B]` | `AND X0, X1, X2` | Bitwise AND of X1 and X2, store in X0 |
| 0x01 | ALU_OR | R | `R[DST] = R[A] | R[B]` | `OR X0, X1, X2` | Bitwise OR of X1 and X2, store in X0 |
| 0x02 | ALU_XOR | R | `R[DST] = R[A] ^ R[B]` | `XOR X0, X1, X2` | Bitwise XOR of X1 and X2, store in X0 |
| 0x03 | ALU_NOT | R | `R[DST] = ~R[A]` | `NOT X0` | Bitwise NOT of X0, store in X0 |
| 0x04 | ALU_ADD | R | `R[DST] = R[A] + R[B]` | `ADD X0, X1, X2` | Add X1 and X2, store sum in X0 |
| 0x05 | ALU_SUB | R | `R[DST] = R[A] - R[B]` | `SUB X0, X1, X2` | Subtract X2 from X1, store in X0 |
| 0x06 | ALU_LSL | R | `R[DST] = R[A] << R[B]` | `LSL X0, X1, X2` | Shift X1 left by X2 bits, store in X0 |
| 0x07 | ALU_LSR | R | `R[DST] = R[A] >> R[B]` | `LSR X0, X1, X2` | Shift X1 right by X2 bits, store in X0 |
| 0x08 | ALU_BCDL | R | `R[DST] = BCD_LOW(R[A])` | `BCDL X0, X1` | Convert X1 to BCD, extract lower 4 digits to X0 |
| 0x09 | ALU_BCDH | R | `R[DST] = BCD_HIGH(R[A])` | `BCDH X0, X1` | Convert X1 to BCD, extract upper 4 digits to X0 |
| 0x0A | ALU_UMUL_L | R | `R[DST] = LOW(R[A] * R[B]) (unsigned)` | `UMUL_L X0, X1, X2` | Unsigned multiply X1 by X2, store lower 16 bits in X0 |
| 0x0B | ALU_UMUL_H | R | `R[DST] = HIGH(R[A] * R[B]) (unsigned)` | `UMUL_H X0, X1, X2` | Unsigned multiply X1 by X2, store upper 16 bits in X0 |
| 0x0C | ALU_MUL_L | R | `R[DST] = LOW(R[A] * R[B]) (signed)` | `MUL_L X0, X1, X2` | Signed multiply X1 by X2, store lower 16 bits in X0 |
| 0x0D | ALU_MUL_H | R | `R[DST] = HIGH(R[A] * R[B]) (signed)` | `MUL_H X0, X1, X2` | Signed multiply X1 by X2, store upper 16 bits in X0 |
| 0x0E | ALU_NUL0E | R | `Reserved ALU 0x0E` | `NUL0E` | Reserved for future ALU operation |
| 0x0F | ALU_NUL0F | R | `Reserved ALU 0x0F` | `NUL0F` | Reserved for future ALU operation |

### Immediate Format [0x10-0x1F]

| OPCODE | Instruction | Format | Description | Usage Example | Behaviour |
|--------|-------------|--------|-------------|---------------|-----------||
| 0x10 | ALU_AND_I | I | `R[DST] = R[A] & IMM` | `AND X0, X1, 0xFF` | Bitwise AND of X1 and 0xFF, store in X0 |
| 0x11 | ALU_OR_I | I | `R[DST] = R[A] | IMM` | `OR X0, X1, 0x10` | Bitwise OR of X1 and 0x10, store in X0 |
| 0x12 | ALU_XOR_I | I | `R[DST] = R[A] ^ IMM` | `XOR X0, X1, 0xFFFF` | Bitwise XOR of X1 and 0xFFFF, store in X0 |
| 0x13 | ALU_NOT_I | I | `R[DST] = ~R[A]` | `NOT X0` | Bitwise NOT of X0, store in X0 |
| 0x14 | ALU_ADD_I | I | `R[DST] = R[A] + IMM` | `ADD X0, X1, 5` | Add X1 and 5, store sum in X0 |
| 0x15 | ALU_SUB_I | I | `R[DST] = R[A] - IMM` | `SUB X0, X1, 10` | Subtract 10 from X1, store in X0 |
| 0x16 | ALU_LSL_I | I | `R[DST] = R[A] << IMM` | `LSL X0, X1, 3` | Shift X1 left by 3 bits, store in X0 |
| 0x17 | ALU_LSR_I | I | `R[DST] = R[A] >> IMM` | `LSR X0, X1, 2` | Shift X1 right by 2 bits, store in X0 |
| 0x18 | ALU_BCDL_I | I | `R[DST] = BCD_LOW(R[A])` | `BCDL X0, X1` | Convert X1 to BCD, extract lower 4 digits to X0 |
| 0x19 | ALU_BCDH_I | I | `R[DST] = BCD_HIGH(R[A])` | `BCDH X0, X1` | Convert X1 to BCD, extract upper 4 digits to X0 |
| 0x1A | ALU_UMUL_L_I | I | `R[DST] = LOW(R[A] * IMM) (unsigned)` | `UMUL_L X0, X1, 3` | Unsigned multiply X1 by 3, store lower 16 bits in X0 |
| 0x1B | ALU_UMUL_H_I | I | `R[DST] = HIGH(R[A] * IMM) (unsigned)` | `UMUL_H X0, X1, 3` | Unsigned multiply X1 by 3, store upper 16 bits in X0 |
| 0x1C | ALU_MUL_L_I | I | `R[DST] = LOW(R[A] * IMM) (signed)` | `MUL_L X0, X1, -2` | Signed multiply X1 by -2, store lower 16 bits in X0 |
| 0x1D | ALU_MUL_H_I | I | `R[DST] = HIGH(R[A] * IMM) (signed)` | `MUL_H X0, X1, -2` | Signed multiply X1 by -2, store upper 16 bits in X0 |
| 0x1E | ALU_NUL0E_I | I | `Reserved ALU 0x1E` | `NUL0E` | Reserved for future ALU operation |
| 0x1F | ALU_NUL0F_I | I | `Reserved ALU 0x1F` | `NUL0F` | Reserved for future ALU operation |

</details>

<details open>
<summary><b>MOV Operations</b></summary>

| OPCODE | Instruction | Format | Description | Usage Example | Behaviour |
|--------|-------------|--------|-------------|---------------|-----------||
| 0x40 | MOVE | R | `R[DST] = R[SRC]` | `MOV X0, X1` | Copy value from X1 to X0 |
| 0x41 | MOVE_I | I | `R[DST] = IMM` | `MOV X0, 100` | Load immediate value 100 into X0 |

</details>


<details open>
<summary><b>Control Flow Operations</b></summary>

### Comparison Operations

| OPCODE | Instruction | Format | Description | Usage Example | Behaviour |
|--------|-------------|--------|-------------|---------------|-----------||
| 0x42 | CMP | R | `FLAGS = R[A] ~ R[B]` | `CMP X0, X1` | Compare X0 and X1, set condition flags |
| 0x43 | CMP_I | I | `FLAGS = R[A] ~ IMM` | `CMP X0, 42` | Compare X0 and 42, set condition flags |

### Branch Operations

| OPCODE | Instruction | Format | Description | Usage Example | Behaviour |
|--------|-------------|--------|-------------|---------------|-----------||
| 0x44 | BRANCH | J | `CONDITION => PC = R[A]` | `BEQ X0` | If condition EQ is true, jump to address in X0 |
| 0x45 | BRANCH_I | JI | `CONDITION => PC = IMM` | `BNE 100` | If condition NE is true, jump to address 100 |

</details>


<details open>
<summary><b>Memory  </b></summary>

| OPCODE | Instruction | Format | Description | Usage Example | Behaviour |
|--------|-------------|--------|-------------|---------------|-----------||
| 0x46 | READ | R | `R[DST] = MEM[R[B]]` | `READ X0, X1` | Load value from memory address in X1 into X0 |
| 0x47 | READ_I | I | `R[DST] = MEM[IMM]` | `READ X0, 50` | Load value from memory address 50 into X0 |
| 0x48 | WRITE | R | `MEM[R[B]] = R[A]` | `WRITE X0, X1` | Store value from X0 to memory address in X1 |
| 0x49 | WRITE_I | I | `MEM[IMM] = R[A]` | `WRITE X0, 50` | Store value from X0 to memory address 50 |

</details>

<details open>
<summary><b>Printing  </b></summary>

In reference to Print instructions of Format `I`, the symbol `Y` refers to the most significant byte of `IMMEDIATE`. Similarly, the symbol `X` refers to least significant byte of `IMMEDIATE`

| OPCODE | Instruction | Format | Description | Usage Example | Behaviour |
|--------|-------------|--------|-------------|---------------|-----------||
| 0x4A | PRINT_REG | R | `SCN[R[B]] = R[A]` | `PRINT X1, X0` | Print value in X1 at screen position in X0 |
| 0x4B | PRINT_REG_I | I | `SCN[IMM] = R[A]` | `PRINT 10, X0` | Print value in X0 at screen position 10 |
| 0x4C | PRINT_CNS | R | `SCN[R[B]] = CONST` | `PRINT X0, 'A'` | Print ASCII 'A' at screen position in X0 |
| 0x4D | PRINT_CNS_I | I | `SCN[IMM] = CONST` | `PRINT 5, 'H'` | Print ASCII 'H' at screen position 5 |

</details>




## Branching (Detailed)

### Branch Conditions

| Code | Mnemonic | Name | Description |
|------|----------|------|-------------|
| 0x0 | B | Unconditional | Always branch |
| 0x1 | BEQ | Equal | Branch if equal (Z set) |
| 0x2 | BNE | Not Equal | Branch if not equal (Z clear) |
| 0x3 | BLT | Less Than | Branch if less than (N set) |
| 0x4 | BLE | Less or Equal | Branch if less than or equal (N set or Z set) |
| 0x5 | BGT | Greater Than | Branch if greater than (N clear and Z clear) |
| 0x6 | BGE | Greater or Equal | Branch if greater than or equal (N clear) |
| 0x7 | BCS | Carry Set | Branch if carry set (C set) |
| 0x8 | BCC | Carry Clear | Branch if carry clear (C clear) |
| 0x9 | BMI | Minus | Branch if negative (N set) |
| 0xA | BPL | Plus | Branch if positive (N clear) |
| 0xB | BVS | Overflow Set | Branch if overflow set (V set) |
| 0xC | BVC | Overflow Clear | Branch if overflow clear (V clear) |
| 0xD | BHI | Higher | Branch if higher (unsigned) |
| 0xE | BLS | Lower or Same | Branch if lower or same (unsigned) |

---

## Register File

- **Count:** 8 general-purpose registers (0-7)
- **Width:** 16-bit

---

## Machine Code Translator ROM

The Machine Code Translator ROM is a 256-entry lookup table that describes instruction properties based on opcode. The address equals the opcode (0x00-0xFF), and the 16-bit value contains flags describing instruction characteristics.

### Bit Flags

| Bit | Name | Description |
|-----|------|-------------|
| 15 | TRY_WRITE | Try write result |
| 14 | TRY_READ_B | Try read B operand |
| 13 | TRY_READ_A | Try read A operand |
| 12 | OVERRIDE_B | OVERRIDE B flag (for ALU_I commands) |
| 11 | OVERRIDE_WRITE | OVERRIDE WRITE flag (for MOV commands) |
| 5 | IMMEDIATE | Is immediate format variant |
| **1-4** | **TYPE** | **Instruction type (bits 1-4)** |
| | TYPE_ALU (0) | ALU Operations |
| | TYPE_FPU (1) | FPU Operations (reserved) |
| | TYPE_MOVE (2) | Move Operations |
| | TYPE_CMP (3) | Comparison Operations |
| | TYPE_BRANCH (4) | Branch/Jump Operations |
| | TYPE_MEMORY (5) | Memory Operations (constant or register addressing) |
| | TYPE_PRINT_REG (6) | PRINT register data (position determined by IMMEDIATE flag) |
| | TYPE_PRINT_CONST (7) | PRINT constant data (position determined by IMMEDIATE flag) |
| | TYPE_SERVICE (8) | Service/System Operations |
| 0 | VALID | Instruction is valid |

---

## Notes

- The ISA is currently in development
- Opcode assignments may change during design phase
