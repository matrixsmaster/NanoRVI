## A very minimalistic implementation of RISC-V 32i ISA emulator, capable of running serious code

Probably the simpleset implementation in the existence. The emulation core is only about 288 lines of pure C code.

I made this implementation to make myself more comfortable with RV32I ISA, and to have a bit of fun. However, I'm now planning to use this in my other projects as well.

### Testing the ISA

Included in `tests/suite` directory, you'll find a version of the [official RISC-V test suite](https://github.com/riscv/riscv-tests) which I modified to run well with my emulator.
Use `do.sh` script to run through all instruction tests automatically. 

___Copyright (C) Dmitry 'MatrixS_Master' Solovyev, 2020___

___This work is licensed under the MIT License. See included LICENSE.TXT___
