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

### Jump Format (J)
```
Bit Layout: OPCODE[8] Condition[4] [0000] A[3]0 [unused]
```
Jumping TODO description

### Jump IMMEDIATE Format (JI)
```
Bit Layout: OPCODE[8] Condition[4] [IMMEDIATE]
```
Jumping TODO description


---

## Operations

<details open>
<summary><b>ALU Operations</b></summary>

### Register Format [0x00-0x0F]

| OPCODE | Instruction | Format | Description |
|--------|-------------|--------|-------------|
| 0x00 | ALU_AND | R | `R[DST] = R[A] & R[B]` |
| 0x01 | ALU_OR | R | `R[DST] = R[A] \| R[B]` |
| 0x02 | ALU_XOR | R | `R[DST] = R[A] ^ R[B]` |
| 0x03 | ALU_NOT | R | `R[DST] = ~R[A]` |
| 0x04 | ALU_ADD | R | `R[DST] = R[A] + R[B]` |
| 0x05 | ALU_SUB | R | `R[DST] = R[A] - R[B]` |
| 0x06 | ALU_LSL | R | `R[DST] = R[A] << 1` |
| 0x07 | ALU_LSR | R | `R[DST] = R[A] >> 1` |
| 0x08 | ALU_BCD_LOW  | R | `R[DST] = X[K]  X[H] X[T] X[O], where X = BCD(R[A])` |
| 0x09 | ALU_BCD_HIGH | R | `R[DST] = X[TK] X[K] X[H] X[T], where X = BCD(R[A])` |
| 0x0A | UNUSED | - | - |
| 0x0B | UNUSED | - | - |
| 0x0C | UNUSED | - | - |
| 0x0D | UNUSED | - | - |
| 0x0E | UNUSED | - | - |
| 0x0F | UNUSED | - | - |

### Immediate Format [0x10-0x1F]

| OPCODE | Instruction | Format | Description |
|--------|-------------|--------|-------------|
| 0x10 | ALU_AND_I | I | `R[DST] = R[A] & IMMEDIATE` |
| 0x11 | ALU_OR_I | I | `R[DST] = R[A] \| IMMEDIATE` |
| 0x12 | ALU_XOR_I | I | `R[DST] = R[A] ^ IMMEDIATE` |
| 0x13 | ALU_NOT_I | I | `R[DST] = ~R[A]` |
| 0x14 | ALU_ADD_I | I | `R[DST] = R[A] + IMMEDIATE` |
| 0x15 | ALU_SUB_I | I | `R[DST] = R[A] - IMMEDIATE` |
| 0x16 | UNUSED | - | - |
| 0x17 | UNUSED | - | - |
| 0x18 | ALU_BCD_LOW_I  | I | `R[DST] = X[K]  X[H] X[T] X[O], where X = BCD(R[A])` |
| 0x19 | ALU_BCD_HIGH_I | I | `R[DST] = X[TK] X[K] X[H] X[T], where X = BCD(R[A])` |
| 0x1A | UNUSED | - | - |
| 0x1B | UNUSED | - | - |
| 0x1C | UNUSED | - | - |
| 0x1D | UNUSED | - | - |
| 0x1E | UNUSED | - | - |
| 0x1F | UNUSED | - | - |

</details>

<details open>
<summary><b>MOV Operations</b></summary>

| OPCODE | Instruction | Format | Description |
|--------|-------------|--------|-------------|
| 0x20 | MOV | R | `R[DST] = R[SRC]` |
| 0x21 | MOV_I | I | `R[DST] = IMMEDIATE` |

</details>


<details open>
<summary><b>Control Flow Operations</b></summary>

### Comparison Operations

| OPCODE | Instruction | Format | Description |
|--------|-------------|--------|-------------|
| 0x22 | CMP    | R | `FLAGS = R[A] ~ R[B]` |
| 0x23 | CMP_I  | I | `FLAGS = R[A] ~ IMMEDIATE` |

### Branch Operations

| OPCODE | Instruction | Format | Description |
|--------|-------------|--------|-------------|
| 0x24 | B   | J | `CONDITION <=> PC = R[A]` |
| 0x25 | B_I | JI | `CONDITION <=> PC = IMMEDIATE` |

</details>



<details open>
<summary><b>Memory  </b></summary>

| OPCODE | Instruction | Format | Description |
|--------|-------------|--------|-------------|
| 0x26 | READ     | R |  `R[DST] = MEM[R[B]]` |
| 0x27 | READ_I   | I |  `R[DST] = MEM[IMMEDIATE[LOWEST FOUR]]` |
| 0x28 | WRITE    | R |  `MEM[R[B]] = R[A]` |
| 0x29 | WRITE_I  | I |  `MEM[IMMEDIATE[LOWEST FOUR]] = R[A]` |

</details>

<details open>
<summary><b>Printing  </b></summary>

In reference to Print instructions of Format `I`, the symbol `Y` refers to the most significant byte of `IMMEDIATE`. Similarly, the symbol `X` refers to least significant byte of `IMMEDIATE`

| OPCODE | Instruction | Format | Description |
|--------|-------------|--------|-------------|
| 0x2A | PRINT_REG      | R |  `SCN[R[B]] = R[A]` |
| 0x2B | RRINT_REG_I    | I |  `SCN[Y] = R[A]`    |
| 0x2C | PRINT_CONST    | R |  `SCN[R[B]] = X` |
| 0x2D | PRINT_CONST_I  | I |  `SCN[Y] = X` |

</details>




## Branching (Detailed)

*(To be expanded with detailed branch condition explanations)*

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
| | TYPE_MOVE (1) | Move Operations |
| | TYPE_CMP (2) | Comparison Operations |
| | TYPE_BRANCH (3) | Branch/Jump Operations |
| | TYPE_MEMORY (4) | Memory Operations (constant or register addressing) |
| | TYPE_PRINT_REG (5) | PRINT register data (position determined by IMMEDIATE flag) |
| | TYPE_PRINT_CONST (6) | PRINT constant data (position determined by IMMEDIATE flag) |
| 0 | VALID | Instruction is valid |

---

## Notes

- The ISA is currently in development
- Opcode assignments may change during design phase
