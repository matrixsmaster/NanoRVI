/*
 *
 * Nano RISC-V 32i emulator
 * Copyright (C) Dmitry 'MatrixS_Master' Solovyev, 2020
 *
 * This work is licensed under the MIT License. See included LICENSE file
 *
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "interface.h"
#include "riscv.h"
#include "debug.h"

#define RAM_BOUNDARY_CHECK(R) rv_interface* iface = (rv_interface*)st->user; \
    if (addr >= iface->ram_size) { \
        iface->error = 1; \
        return R; \
    }

// RAM read access functions
static uint32_t read8(riscv_state* st, uint32_t addr)
{
    RAM_BOUNDARY_CHECK(0)
    uint8_t* ptr = iface->ram + addr;
    uint32_t val = *ptr;
    if (iface->debug & DBG_MEM) printf("Read byte from 0x%08X: 0x%02X\n",addr,val);
    return val;
}

static uint32_t read16(riscv_state* st, uint32_t addr)
{
    RAM_BOUNDARY_CHECK(0)
    uint16_t* ptr = (uint16_t*)(iface->ram + addr);
    uint32_t val = *ptr;
    if (iface->debug & DBG_MEM) printf("Read half-word from 0x%08X: 0x%02X\n",addr,val);
    return val;
}

static uint32_t read32(riscv_state* st, uint32_t addr)
{
    RAM_BOUNDARY_CHECK(0)
    uint32_t* ptr = (uint32_t*)(iface->ram + addr);
    uint32_t val = *ptr;
    if (iface->debug & DBG_MEM) printf("Read word from 0x%08X: 0x%02X\n",addr,val);
    return val;
}

// RAM write access functions
static void write8(riscv_state* st, uint32_t addr, uint32_t val)
{
    RAM_BOUNDARY_CHECK()
    uint8_t* ptr = iface->ram + addr;
    *ptr = val & 0xFF;
    if (iface->debug & DBG_MEM) printf("Write byte to 0x%08X: 0x%02X\n",addr,val);
}

static void write16(riscv_state* st, uint32_t addr, uint32_t val)
{
    RAM_BOUNDARY_CHECK()
    uint16_t* ptr = (uint16_t*)(iface->ram + addr);
    *ptr = val & 0xFFFF;
    if (iface->debug & DBG_MEM) printf("Write half-word to 0x%08X: 0x%02X\n",addr,val);
}

static void write32(riscv_state* st, uint32_t addr, uint32_t val)
{
    RAM_BOUNDARY_CHECK()
    uint32_t* ptr = (uint32_t*)(iface->ram + addr);
    *ptr = val;
    if (iface->debug & DBG_MEM) printf("Write word to 0x%08X: 0x%02X\n",addr,val);
}

// ECALL (a.k.a. SYSCALL) instruction implementation
static uint8_t ecall(riscv_state* st)
{
    rv_interface* iface = (rv_interface*)st->user;

    // trace - syscalls
    if (iface->debug & DBG_SYSCALL)
        printf("Syscall request %u encountered at ip=0x%08X\n",st->regs[RVR_A7],st->ip);

    // execute known syscall
    switch (st->regs[RVR_A7]) {
    case RVSYS_CLOSE:
        //TODO
        st->regs[RVR_A0] = 0; // success
        break;

    case RVSYS_WRITE:
        for (unsigned j = 0; j < st->regs[RVR_A2]; j++) putchar(read8(st,st->regs[RVR_A1]+j));
        st->regs[RVR_A0] = st->regs[RVR_A2]; // return length field
        break;

    case RVSYS_FSTAT:
        //TODO
        st->regs[RVR_A0] = 0; // success
        break;

    case RVSYS_EXIT:
        if (iface->debug & DBG_SYSCALL) printf("Exiting with code %u\n",st->regs[RVR_A0]);
        return 1;

    case RVSYS_BRK:
        if (iface->debug & DBG_SYSCALL)
            printf("Moving program break to 0x%08X\n",st->regs[RVR_A0]);
        if (st->regs[RVR_A0] && st->regs[RVR_A0] < iface->heap_max)
            iface->prog_break = st->regs[RVR_A0];
        st->regs[RVR_A0] = iface->prog_break;
        break;

    default:
        printf("WARNING: Unimplemented syscall %d\n",st->regs[RVR_A7]);
    }

    return 0;
}

// EBREAK instruction implementation
static void ebreak(riscv_state* st)
{
    printf("Breakpoint encountered at ip=0x%08X\nPress Enter to continue\n",st->ip);
    // simply stop there, probably I'll fit some debug output later
    getchar();
}

void rv_iface_init(rv_interface* iface)
{
    memset(iface,0,sizeof(rv_interface));
}

bool rv_iface_start(rv_interface* iface)
{
    iface->vm.user = iface; // create circular pointer

    // Fill in all the callbacks
    iface->vm.funcs.read8 = read8;
    iface->vm.funcs.read16 = read16;
    iface->vm.funcs.read32 = read32;
    iface->vm.funcs.write8 = write8;
    iface->vm.funcs.write16 = write16;
    iface->vm.funcs.write32 = write32;
    iface->vm.funcs.ecall = ecall;
    iface->vm.funcs.ebreak = ebreak;

    // Allocate RAM
    iface->ram = (uint8_t*)malloc(iface->ram_size);
    if (!iface->ram) {
        printf("ERROR: Unable to allocate %u bytes of RAM\n",iface->ram_size);
        return false;
    }

    // If stack bottom is still not initialized, set it to the end of RAM
    if (!iface->stack_start) iface->stack_start = iface->ram_size - 4;

    // Set IP & SP
    iface->vm.ip = iface->start;
    iface->vm.regs[RVR_SP] = iface->stack_start;

    // Set other limits
    iface->heap_max = iface->ram_size - iface->stack_size;

    return true;
}

bool rv_iface_step(rv_interface* iface)
{
    // trace - part 1
    if (iface->debug & DBG_TRACE) {
        char buf[IFACE_DISASM_MAX_LEN];
        riscv_exit r = riscv_disasm(iface->ram[iface->vm.ip],buf,sizeof(buf));
        if (r == RVEXIT_SUCCESS)
            printf("0x%08X: %s\n",iface->vm.ip,buf);
    }

    // trace - part 2, registers
    if (iface->debug & DBG_REGS) {
        for (int k = 1; k < 32; k++) printf("%d ",iface->vm.regs[k]);
        puts("");
    }

    // trace - part 3, interactive wait
    if (iface->debug & DBG_INTERACTIVE) getchar();

    // actual instruction execution :)
    riscv_exit ret = riscv_exec(&(iface->vm));

    // check for errors
    if (iface->error) {
        printf("ERROR: execution error %u\n",iface->error);
        return false;

    } else
        return ret == RVEXIT_SUCCESS;
}

void rv_iface_stop(rv_interface* iface)
{
    if (iface->ram) free(iface->ram);
}
