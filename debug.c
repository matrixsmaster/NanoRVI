/*
 *
 * Nano RISC-V 32i emulator
 * Copyright (C) Dmitry 'MatrixS_Master' Solovyev, 2020-2021
 *
 * This work is licensed under the MIT License. See included LICENSE file
 *
 * */

#include "debug.h"

static const struct {
    char opt;
    uint32_t flag;
} debug_switches[] = {
        { 't', DBG_TRACE },
        { 's', DBG_SYSCALL },
        { 'm', DBG_MEM },
        { 'r', DBG_REGS },
        { 'i', DBG_INTERACTIVE },
        { 'l', DBG_LOAD },
        { 0, 0 }
};

// Read command line argument and parse debug options
uint32_t debug_readopts(const char* arg)
{
    uint32_t ret = 0;
    while (*arg) {
        for (int i = 0; debug_switches[i].opt; i++) {
            if (*arg == debug_switches[i].opt) {
                ret |= debug_switches[i].flag;
                break;
            }
        }
        arg++;
    }
    return ret;
}
