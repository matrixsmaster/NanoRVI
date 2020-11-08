/*
 *
 * Nano RISC-V 32i emulator
 * Copyright (C) Dmitry 'MatrixS_Master' Solovyev, 2020
 *
 * This work is licensed under the MIT License. See included LICENSE file
 *
 * */

#ifndef INTERFACE_H_
#define INTERFACE_H_

#include <stdbool.h>
#include <inttypes.h>
#include "riscv.h"

#define IFACE_DISASM_MAX_LEN 356

// Virtual machine state structure
typedef struct {
    riscv_state vm;
    uint8_t* ram;
    uint32_t ram_size;
    uint32_t stack_size;
    uint32_t stack_start;
    uint32_t prog_break;
    uint32_t heap_max;
    uint32_t start;
    uint32_t debug;
    uint32_t error;
} rv_interface;

// Syscall codes (see "syscall.h" for values)
enum rv_syscall {
    RVSYS_CLOSE = 57,
    RVSYS_WRITE = 64,
    RVSYS_FSTAT = 80,
    RVSYS_EXIT = 93,
    RVSYS_BRK = 214,
};

void rv_iface_init(rv_interface* iface);
bool rv_iface_resize(rv_interface* iface);
void rv_iface_start(rv_interface* iface);
bool rv_iface_step(rv_interface* iface);
void rv_iface_stop(rv_interface* iface);

#endif /* INTERFACE_H_ */
