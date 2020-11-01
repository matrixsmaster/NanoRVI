/*
 *
 * Nano RISC-V 32i emulator
 * Copyright (C) Dmitry 'MatrixS_Master' Solovyev, 2020
 *
 * This work is licensed under the MIT License. See included LICENSE.TXT.
 *
 * */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include "riscv.h"
#include "elf.h"

#define RAMSIZE (40*1024*1024)
#define STACKSIZE (20*1024*1024)
#define HEAPSIZE (RAMSIZE-STACKSIZE)

enum debug_opts {
    DBG_TRACE = 0x01,
    DBG_SYSCALL = 0x02,
    DBG_MEM = 0x04,
    DBG_REGS = 0x08,
    DBG_INTERACTIVE = 0x10,
    DBG_LOAD = 0x20,
};

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

uint32_t ip = 0;
uint32_t regs[RV_NUMREGS] = {0};
uint8_t ram[RAMSIZE] = {0};
uint32_t code_end = 0;
uint32_t debug = 0;

void readopts(const char* arg)
{
    while (*arg) {
        for (int i = 0; debug_switches[i].opt; i++) {
            if (*arg == debug_switches[i].opt) {
                debug |= debug_switches[i].flag;
                break;
            }
        }
        arg++;
    }
}

uint32_t readelf(const char* fn)
{
    FILE* f = fopen(fn,"rb");
    assert(f);

    elf_header_t elfhdr;
    assert(fread(&elfhdr,sizeof(elfhdr),1,f));

    const char magic[] = "\x7f" "ELF";
    assert(!memcmp(magic,elfhdr.magic,4));
    assert(elfhdr.ver == 1);
    assert(elfhdr.class == 1);
    assert(elfhdr.endianness == 1);
    assert(elfhdr.machine == 0xF3);

    fseek(f,elfhdr.proghdr_off,SEEK_SET);

    for (uint16_t i = 0; i < elfhdr.proghdr_num; i++) {
        elf_proghdr_t phdr;
        assert(fread(&phdr,sizeof(phdr),1,f));

        code_end = phdr.vaddr + phdr.memsz;
        if (code_end >= RAMSIZE) {
            printf("ELF Section %u is too big (0x%08X - 0x%08X) to fit in RAM\n",i,phdr.vaddr,code_end);
            return 0;
        }

        size_t pos = ftell(f);
        fseek(f,phdr.off,SEEK_SET);
        assert(fread(ram+phdr.vaddr,phdr.filesz,1,f));
        fseek(f,pos,SEEK_SET);

        if (debug & DBG_LOAD)
            printf("Section %u loaded, 0x%08X - 0x%08X, %u bytes\n",i,phdr.vaddr,code_end,phdr.memsz);
    }

    fclose(f);

    return elfhdr.entry;
}

riscv_op decode(uint32_t in, uint32_t* imm)
{
    for (int i = 0; i <= RV_EBREAK; i++) {
        uint32_t tmp = 0;
        int fnd = 1;
        for (int j = 0; (j < 32) && fnd; j++) {
            if (riscv_encode[i][j] == ' ') continue;
            if (riscv_encode[i][j] >= '<') {
                uint32_t d = (in & (1U << (31-j)))? 1:0;
                d <<= riscv_encode[i][j] - '<';
                tmp |= d;
            } else {
                char d = (in & (1U << (31-j)))? '1':'0';
                if (riscv_encode[i][j] != d) {
                    fnd = 0;
                }
            }
        }
        if (fnd) {
            *imm = tmp;
            return i;
        }
    }

    printf("Unable to decode instruction 0x%08X @ 0x%08X\n",in,ip);
    for (int i = 0; i < 32; i++, in<<=1) putchar((in & 0x80000000)? '1':'0');
    putchar('\n');

    return RV_EBREAK + 1; //invalid op
}

uint32_t read8(uint32_t addr, int sign)
{
    assert(addr < RAMSIZE);
    uint8_t* ptr = ram + addr;
    uint32_t val = sign? (uint32_t)RV_EXTEND(*ptr,7) : (uint32_t)(*ptr);
    if (debug & DBG_MEM) printf("Read %s byte from 0x%08X: 0x%02X\n",(sign? "signed":"unsigned"),addr,val);
    return val;
}

uint32_t read16(uint32_t addr, int sign)
{
    assert(addr < RAMSIZE);
    uint16_t* ptr = (uint16_t*)(ram + addr);
    uint32_t val = sign? (uint32_t)RV_EXTEND(*ptr,15) : (uint32_t)(*ptr);
    if (debug & DBG_MEM) printf("Read %s half-word from 0x%08X: 0x%02X\n",(sign? "signed":"unsigned"),addr,val);
    return val;
}

uint32_t read32(uint32_t addr)
{
    assert(addr < RAMSIZE);
    uint32_t* ptr = (uint32_t*)(ram + addr);
    uint32_t val = *ptr;
    if (debug & DBG_MEM) printf("Read word from 0x%08X: 0x%02X\n",addr,val);
    return val;
}

void write8(uint32_t addr, uint32_t val)
{
    assert(addr < RAMSIZE);
    uint8_t* ptr = ram + addr;
    *ptr = val & 0xFF;
    if (debug & DBG_MEM) printf("Write byte to 0x%08X: 0x%02X\n",addr,val);
}

void write16(uint32_t addr, uint32_t val)
{
    assert(addr < RAMSIZE);
    uint16_t* ptr = (uint16_t*)(ram + addr);
    *ptr = val & 0xFFFF;
    if (debug & DBG_MEM) printf("Write half-word to 0x%08X: 0x%02X\n",addr,val);
}

void write32(uint32_t addr, uint32_t val)
{
    assert(addr < RAMSIZE);
    uint32_t* ptr = (uint32_t*)(ram + addr);
    *ptr = val;
    if (debug & DBG_MEM) printf("Write word to 0x%08X: 0x%02X\n",addr,val);
}

uint8_t ecall()
{
    if (debug & DBG_SYSCALL)
        printf("Syscall request %u encountered at ip=0x%08X\n",regs[RVR_A7],ip);

    switch (regs[RVR_A7]) {
    case RVSYS_CLOSE:
        //TODO
        regs[RVR_A0] = 0; // success
        break;

    case RVSYS_WRITE:
        for (unsigned j = 0; j < regs[RVR_A2]; j++) putchar(read8(regs[RVR_A1]+j,0));
        regs[RVR_A0] = regs[RVR_A2]; // return length field
        break;

    case RVSYS_FSTAT:
        //TODO
        regs[RVR_A0] = 0; // success
        break;

    case RVSYS_EXIT:
        if (debug & DBG_SYSCALL) printf("Exiting with code %u\n",regs[RVR_A0]);
        return 1;

    case RVSYS_BRK:
        if (debug & DBG_SYSCALL)
            printf("Moving program break to 0x%08X\n",regs[RVR_A0]);
        if (regs[RVR_A0] && regs[RVR_A0] < HEAPSIZE)
            code_end = regs[RVR_A0];
        regs[RVR_A0] = code_end;
        break;

    default:
        printf("Unimplemented syscall %d\n",regs[RVR_A7]);
    }

    return 0;
}

void ebreak()
{
    printf("Breakpoint encountered at ip=0x%08X\nPress Enter to continue\n",ip);
    getchar();
}

int main(int argc, char* argv[])
{
    assert(argc > 1);

    if (argc > 2) readopts(argv[2]);

    ip = readelf(argv[1]);
    assert(ip);

    regs[RVR_SP] = RAMSIZE - 4;

    while (ip < RAMSIZE) {
        assert((ip & 3) == 0);
        regs[RVR_ZERO] = 0;

        uint32_t inst = read32(ip);
        uint32_t imm = 0;
        uint32_t rd = (inst >> 7) & 0x1F;
        uint32_t rs1 = (inst >> 15) & 0x1F;
        uint32_t rs2 = (inst >> 20) & 0x1F;
        riscv_op op = decode(inst,&imm);
        assert(op <= RV_EBREAK);

        if (debug & DBG_TRACE) {
            printf("0x%08X: %s ",ip,riscv_names[op]);
            if (riscv_useregs[op][0] == '1')
                printf("%s ",riscv_regname[rd]);
            if (riscv_useregs[op][1] == '1')
                printf("%s ",riscv_regname[rs1]);
            if (riscv_useregs[op][2] == '1')
                printf("%s ",riscv_regname[rs2]);
            printf("(0x%06X) SP=%d (0x%08X)\n",imm,regs[RVR_SP],regs[RVR_SP]);
        }

        if (debug & DBG_REGS) {
            for (int k = 1; k < 32; k++) printf("%d ",regs[k]);
            puts("");
        }

        if (debug & DBG_INTERACTIVE) getchar();

        uint8_t shf;
        uint32_t tmp;
        uint32_t jmp = 0;
        switch (op) {
        case RV_LUI:
            regs[rd] = imm;
            break;
        case RV_AUIPC:
            regs[rd] = ip + imm;
            break;
        case RV_JAL:
            if (rd) regs[rd] = ip + 4;
            ip += RV_EXTEND(imm,20);
            jmp = 1;
            break;
        case RV_JALR:
            tmp = ip;
            ip = regs[rs1] + RV_EXTEND(imm,11);
            if (rd) regs[rd] = tmp + 4;
            jmp = 1;
            break;
        case RV_BEQ:
            ip += (regs[rs1] == regs[rs2])? RV_EXTEND(imm,12):4;
            jmp = 1;
            break;
        case RV_BNE:
            ip += (regs[rs1] != regs[rs2])? RV_EXTEND(imm,12):4;
            jmp = 1;
            break;
        case RV_BLT:
            ip += ((int32_t)(regs[rs1]) < (int32_t)(regs[rs2]))? RV_EXTEND(imm,12):4;
            jmp = 1;
            break;
        case RV_BGE:
            ip += ((int32_t)(regs[rs1]) >= (int32_t)(regs[rs2]))? RV_EXTEND(imm,12):4;
            jmp = 1;
            break;
        case RV_BLTU:
            ip += (regs[rs1] < regs[rs2])? RV_EXTEND(imm,12):4;
            jmp = 1;
            break;
        case RV_BGEU:
            ip += (regs[rs1] >= regs[rs2])? RV_EXTEND(imm,12):4;
            jmp = 1;
            break;
        case RV_LB:
            regs[rd] = read8(regs[rs1]+RV_EXTEND(imm,11),1);
            break;
        case RV_LH:
            regs[rd] = read16(regs[rs1]+RV_EXTEND(imm,11),1);
            break;
        case RV_LW:
            regs[rd] = read32(regs[rs1]+RV_EXTEND(imm,11));
            break;
        case RV_LBU:
            regs[rd] = read8(regs[rs1]+RV_EXTEND(imm,11),0);
            break;
        case RV_LHU:
            regs[rd] = read16(regs[rs1]+RV_EXTEND(imm,11),0);
            break;
        case RV_SB:
            write8(regs[rs1]+RV_EXTEND(imm,11),regs[rs2]);
            break;
        case RV_SH:
            write16(regs[rs1]+RV_EXTEND(imm,11),regs[rs2]);
            break;
        case RV_SW:
            write32(regs[rs1]+RV_EXTEND(imm,11),regs[rs2]);
            break;
        case RV_ADDI:
            regs[rd] = (uint32_t)((int32_t)(regs[rs1]) + RV_EXTEND(imm,11));
            break;
        case RV_SLTI:
            regs[rd] = ((int32_t)(regs[rs1]) < RV_EXTEND(imm,11))? 1:0;
            break;
        case RV_SLTIU:
            regs[rd] = (regs[rs1] < (uint32_t)(RV_EXTEND(imm,11)))? 1:0;
            break;
        case RV_XORI:
            regs[rd] = regs[rs1] ^ RV_EXTEND(imm,11);
            break;
        case RV_ORI:
            regs[rd] = regs[rs1] | RV_EXTEND(imm,11);
            break;
        case RV_ANDI:
            regs[rd] = regs[rs1] & RV_EXTEND(imm,11);
            break;
        case RV_SLLI:
            regs[rd] = regs[rs1] << rs2;
            break;
        case RV_SRLI:
            regs[rd] = regs[rs1] >> rs2;
            break;
        case RV_SRAI:
            tmp = (regs[rs1] & 0x80000000)? ((1U << rs2) - 1) << (32 - rs2) : 0;
            regs[rd] = (regs[rs1] >> rs2) | tmp;
            break;
        case RV_ADD:
            regs[rd] = regs[rs1] + regs[rs2];
            break;
        case RV_SUB:
            regs[rd] = regs[rs1] - regs[rs2];
            break;
        case RV_SLL:
            regs[rd] = regs[rs1] << (regs[rs2] & 0x1F);
            break;
        case RV_SLT:
            regs[rd] = ((int32_t)(regs[rs1]) < (int32_t)(regs[rs2]))? 1:0;
            break;
        case RV_SLTU:
            regs[rd] = (regs[rs1] < regs[rs2])? 1:0;
            break;
        case RV_XOR:
            regs[rd] = regs[rs1] ^ regs[rs2];
            break;
        case RV_SRL:
            regs[rd] = regs[rs1] >> (regs[rs2] & 0x1F);
            break;
        case RV_SRA:
            shf = regs[rs2] & 0x1F;
            tmp = (regs[rs1] & 0x80000000)? ((1U << shf) - 1) << (32 - shf) : 0;
            regs[rd] = (regs[rs1] >> shf) | tmp;
            break;
        case RV_OR:
            regs[rd] = regs[rs1] | regs[rs2];
            break;
        case RV_AND:
            regs[rd] = regs[rs1] & regs[rs2];
            break;
        case RV_FENCE:
            break;
        case RV_ECALL:
            if (ecall()) ip = RAMSIZE;
            break;
        case RV_EBREAK:
            ebreak();
            break;
        }

        if (!jmp) ip += 4;
    }

    puts("Quit");
}
