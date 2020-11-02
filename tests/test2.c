/*
 * riscv32-unknown-elf-gcc -march=rv32i -mabi=ilp32 -Wl,-gc-sections -g0 -o test2.elf test2.c
 * */
#include <stdio.h>
int main()
{ puts("Hello world"); return 0; }
