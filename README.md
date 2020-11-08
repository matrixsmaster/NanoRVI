## NanoRVI - a very minimalistic implementation of RISC-V 32i ISA emulator, capable of running serious code

Probably the simpleset implementation in the existence. The emulation core is only about 288 lines of pure C code. And it's completely embeddable into your own projects.

I made this implementation to make myself more comfortable with RV32I ISA, and to have a bit of fun. However, I'm now planning to use this in my other projects as well.

### Testing the ISA

Included in `tests/suite` directory, you'll find a version of the [official RISC-V test suite](https://github.com/riscv/riscv-tests) which I modified to run well with my emulator.
Use `do.sh` script to run through all instruction tests automatically.

### Reusing the code

If you want to embed this emulator into your own project, all you need to do is:

1. Copy 3 files (riscv.c; riscv.h; riscv_tabs.h) into your project source tree
2. Implement virtual memory read/write functions:

    1. `read(8/16/32)` - simple __unsigned__ read
    2. `write(8/16/32)` - simple __unsigned__ write

3. Implement service functions:

    1. `ecall` - syscall (consult RISC-V toolchain's syscall.h)
    2. `ebreak` - your breakpoint implementation (just an empty function in the simplest case)

4. Initialize `riscv_state` structure and use it when calling `riscv_exec()`

The emulator core is completely re-entrant, so you can enjoy running thousands of virtual RISC-V CPUs in parallel on your mighty GPU ;)

___Copyright (C) Dmitry 'MatrixS_Master' Solovyev, 2020___

___This work is licensed under the MIT License. See included LICENSE file___
