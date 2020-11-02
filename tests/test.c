/*
 * riscv32-unknown-elf-gcc -march=rv32i -mabi=ilp32 -Wl,-gc-sections -g0 -o test.elf test.c
  * */
int main()
{
    volatile int a = 1;
    a++;
    return a;
}
