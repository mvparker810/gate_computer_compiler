# LSASM - Logic Simulator Assembler

An assembler for a custom 8-bit instruction set architecture. Compiles assembly-style code into machine code.

Built for the custom computer from [Digital Logic Sim by Sebastian Lague](https://sebastian.itch.io/digital-logic-sim).

More info at [mvparker.dev](https://mvparker.dev)

## Building

```bash
gcc lsasm.c -o lsasm
```

## Usage

```bash
./lsasm [-f FORMAT] <input_file> <output_file>
```

### Options

- `-f <FORMAT>` - Output format: `hex`, `uint`, `int`, `binary` (default: `hex`)

### Examples

```bash
./lsasm scripts/fib.txt machine_code.out
./lsasm -f binary scripts/fib.txt machine_code.out
./lsasm -f uint input.txt output.txt
```

## Instruction Set

**ALU Operations:** AND, OR, XOR, NOT, ADD, SUB, LSL, LSR

**Memory:** READ, WRITE (constant/register addressing)

**Control Flow:** B, BEQ, BNE, BLT, BLE, BGT, BGE, BCS, BCC, BMI, BPL, BVS, BVC, BHI, BLS

**Other:** MOV, CMP, EXIT
