# vm8086
Intel 8086 instructions decoder. Takes a 8086 binary program and inputs its as an assembly representation. Basically a disassembler. Currently supports the following instructions in both 8-bit and 16-bit formats:

 * `MOV` - register to register, register to memory, memory to register
 * `ADD` - immediate to accumulator, register to memory, memory to register, register to register
 * `CMP` - immediate with accumulator
 * `SUB` - immediate from accumulator, register from memory, memory from register, register to register
 * `JUMP` - including je, jz, jl, jnge, jle, jng, jb, jnae and others.
 * `LOOP` - including loopz, loopnz and others.

## Requirements

 * UNIX system - MacOS, Linux

## Usage
```bash
# you can take assembled program from /resources folder
# and supply it to the standard input of the program
cat /resources/<program> | vm8086
# assembly represenation will be printed to standard output
```

## Resources
 * [Intel 8086 User Manual](https://edge.edx.org/c4x/BITSPilani/EEE231/asset/8086_family_Users_Manual_1_.pdf)
 * [Course "Performance Aware Programming" by Casey Muratori](https://www.computerenhance.com)

