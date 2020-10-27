/*
 * riscv32-unknown-elf-gcc -march=rv32i -mabi=ilp32 -Wl,-gc-sections -g0 -o test2.elf test2.c
 * riscv32-unknown-elf-strip test2.elf
 * riscv32-unknown-elf-objcopy -S -O ihex test2.elf test2.hex
 * */
#include <stdio.h>
int main()
{ puts("Hello world"); return 0; }
