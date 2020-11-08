/*
 *
 * Nano RISC-V 32i emulator
 * Copyright (C) Dmitry 'MatrixS_Master' Solovyev, 2020
 *
 * This work is licensed under the MIT License. See included LICENSE file
 *
 * */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <inttypes.h>

enum debug_opts {
    DBG_TRACE = 0x01,
    DBG_SYSCALL = 0x02,
    DBG_MEM = 0x04,
    DBG_REGS = 0x08,
    DBG_INTERACTIVE = 0x10,
    DBG_LOAD = 0x20,
};

uint32_t debug_readopts(const char* arg);

#endif /* DEBUG_H_ */
