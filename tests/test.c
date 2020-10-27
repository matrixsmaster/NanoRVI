/*
 * export PATH="/home/lisa/local/riscv-gcc/bin:$PATH"
 * riscv32-unknown-elf-gcc -march=rv32i -mabi=ilp32 -o test.elf test.c
 * riscv32-unknown-elf-objcopy -S -O ihex test.elf test.hex
  * */
int main()
{
    volatile int a = 1;
    a++;
    return a;
}
