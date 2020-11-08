/*
 *
 * Nano RISC-V 32i emulator
 * Copyright (C) Dmitry 'MatrixS_Master' Solovyev, 2020
 *
 * This work is licensed under the MIT License. See included LICENSE file
 *
 * */

#include <stdio.h>
#include <assert.h>
#include "riscv.h"
#include "riscv_tabs.h"

// Decode an instruction and extract its immediate argument at the same time
static riscv_op decode(uint32_t in, uint32_t* imm)
{
    // for each opcode template in the 'riscv_encode' table
    for (int i = 0; i <= RV_EBREAK; i++) {
        uint32_t tmp = 0;
        int fnd = 1;

        // go from LSB to MSB and compare known bits
        for (int j = 31; (j >= 0) && fnd; j--) {
            // skip fields
            if (riscv_encode[i][j] == RV_ENCODE_SYM_DONT_CARE) continue;

            if (riscv_encode[i][j] >= RV_ENCODE_SYM_IMM_START) {
                // combine bits of immediate argument
                uint32_t d = (in & (1U << (31-j)))? 1:0;
                d <<= riscv_encode[i][j] - RV_ENCODE_SYM_IMM_START;
                tmp |= d;

            } else {
                // compare static (fixed) bits
                char d = (in & (1U << (31-j)))? '1':'0';
                if (riscv_encode[i][j] != d) {
                    fnd = 0;
                }
            }
        }

        // we found our opcode and extracted immediate argument
        if (fnd) {
            *imm = tmp;
            return i;
        }
    }

    return RV_EBREAK + 1; //invalid op
}

// Helper memory interface functions to help with signed/unsigned readings
static uint32_t read8(riscv_state* st, uint32_t addr, int sign)
{
    uint8_t val = st->funcs.read8(st,addr);
    return sign? (uint32_t)RV_EXTEND(val,7) : (uint32_t)val;
}

static uint32_t read16(riscv_state* st, uint32_t addr, int sign)
{
    uint16_t val = st->funcs.read16(st,addr);
    return sign? (uint32_t)RV_EXTEND(val,15) : (uint32_t)val;
}

// Simply execute RISC-V instructions
riscv_exit riscv_exec(riscv_state* st)
{
    assert((st->ip & 3) == 0); // sanity check
    st->regs[RVR_ZERO] = 0; // to simplify things, x0 is just a regular register

    // read next 32-bit instruction, extract known fields and decode the opcode
    uint32_t inst = st->funcs.read32(st,st->ip);
    uint32_t imm = 0;
    uint32_t rd = (inst >> 7) & 0x1F;
    uint32_t rs1 = (inst >> 15) & 0x1F;
    uint32_t rs2 = (inst >> 20) & 0x1F;
    riscv_op op = decode(inst,&imm);

    if (op > RV_EBREAK) {
        // dunno what was that
        printf("Unable to decode instruction 0x%08X @ 0x%08X\n",inst,st->ip);
        for (int i = 0; i < 32; i++, inst <<= 1) putchar((inst & 0x80000000)? '1':'0');
        putchar('\n');
        return RVEXIT_WRONGOPCODE;
    }

    // execute instruction according to riscv-spec-20191213
    uint8_t shf, jmp = 0, end = 0;
    uint32_t tmp;
    switch (op) {
    case RV_LUI:
        st->regs[rd] = imm;
        break;
    case RV_AUIPC:
        st->regs[rd] = st->ip + imm;
        break;
    case RV_JAL:
        if (rd) st->regs[rd] = st->ip + 4;
        st->ip += RV_EXTEND(imm,20);
        jmp = 1;
        break;
    case RV_JALR:
        tmp = st->ip;
        st->ip = st->regs[rs1] + RV_EXTEND(imm,11);
        if (rd) st->regs[rd] = tmp + 4;
        jmp = 1;
        break;
    case RV_BEQ:
        st->ip += (st->regs[rs1] == st->regs[rs2])? RV_EXTEND(imm,12):4;
        jmp = 1;
        break;
    case RV_BNE:
        st->ip += (st->regs[rs1] != st->regs[rs2])? RV_EXTEND(imm,12):4;
        jmp = 1;
        break;
    case RV_BLT:
        st->ip += ((int32_t)(st->regs[rs1]) < (int32_t)(st->regs[rs2]))? RV_EXTEND(imm,12):4;
        jmp = 1;
        break;
    case RV_BGE:
        st->ip += ((int32_t)(st->regs[rs1]) >= (int32_t)(st->regs[rs2]))? RV_EXTEND(imm,12):4;
        jmp = 1;
        break;
    case RV_BLTU:
        st->ip += (st->regs[rs1] < st->regs[rs2])? RV_EXTEND(imm,12):4;
        jmp = 1;
        break;
    case RV_BGEU:
        st->ip += (st->regs[rs1] >= st->regs[rs2])? RV_EXTEND(imm,12):4;
        jmp = 1;
        break;
    case RV_LB:
        st->regs[rd] = read8(st,st->regs[rs1]+RV_EXTEND(imm,11),1);
        break;
    case RV_LH:
        st->regs[rd] = read16(st,st->regs[rs1]+RV_EXTEND(imm,11),1);
        break;
    case RV_LW:
        st->regs[rd] = st->funcs.read32(st,st->regs[rs1]+RV_EXTEND(imm,11));
        break;
    case RV_LBU:
        st->regs[rd] = read8(st,st->regs[rs1]+RV_EXTEND(imm,11),0);
        break;
    case RV_LHU:
        st->regs[rd] = read16(st,st->regs[rs1]+RV_EXTEND(imm,11),0);
        break;
    case RV_SB:
        st->funcs.write8(st,st->regs[rs1]+RV_EXTEND(imm,11),st->regs[rs2]);
        break;
    case RV_SH:
        st->funcs.write16(st,st->regs[rs1]+RV_EXTEND(imm,11),st->regs[rs2]);
        break;
    case RV_SW:
        st->funcs.write32(st,st->regs[rs1]+RV_EXTEND(imm,11),st->regs[rs2]);
        break;
    case RV_ADDI:
        st->regs[rd] = (uint32_t)((int32_t)(st->regs[rs1]) + RV_EXTEND(imm,11));
        break;
    case RV_SLTI:
        st->regs[rd] = ((int32_t)(st->regs[rs1]) < RV_EXTEND(imm,11))? 1:0;
        break;
    case RV_SLTIU:
        st->regs[rd] = (st->regs[rs1] < (uint32_t)(RV_EXTEND(imm,11)))? 1:0;
        break;
    case RV_XORI:
        st->regs[rd] = st->regs[rs1] ^ RV_EXTEND(imm,11);
        break;
    case RV_ORI:
        st->regs[rd] = st->regs[rs1] | RV_EXTEND(imm,11);
        break;
    case RV_ANDI:
        st->regs[rd] = st->regs[rs1] & RV_EXTEND(imm,11);
        break;
    case RV_SLLI:
        st->regs[rd] = st->regs[rs1] << rs2;
        break;
    case RV_SRLI:
        st->regs[rd] = st->regs[rs1] >> rs2;
        break;
    case RV_SRAI:
        tmp = (st->regs[rs1] & 0x80000000)? ((1U << rs2) - 1) << (32 - rs2) : 0;
        st->regs[rd] = (st->regs[rs1] >> rs2) | tmp;
        break;
    case RV_ADD:
        st->regs[rd] = st->regs[rs1] + st->regs[rs2];
        break;
    case RV_SUB:
        st->regs[rd] = st->regs[rs1] - st->regs[rs2];
        break;
    case RV_SLL:
        st->regs[rd] = st->regs[rs1] << (st->regs[rs2] & 0x1F);
        break;
    case RV_SLT:
        st->regs[rd] = ((int32_t)(st->regs[rs1]) < (int32_t)(st->regs[rs2]))? 1:0;
        break;
    case RV_SLTU:
        st->regs[rd] = (st->regs[rs1] < st->regs[rs2])? 1:0;
        break;
    case RV_XOR:
        st->regs[rd] = st->regs[rs1] ^ st->regs[rs2];
        break;
    case RV_SRL:
        st->regs[rd] = st->regs[rs1] >> (st->regs[rs2] & 0x1F);
        break;
    case RV_SRA:
        shf = st->regs[rs2] & 0x1F;
        tmp = (st->regs[rs1] & 0x80000000)? ((1U << shf) - 1) << (32 - shf) : 0;
        st->regs[rd] = (st->regs[rs1] >> shf) | tmp;
        break;
    case RV_OR:
        st->regs[rd] = st->regs[rs1] | st->regs[rs2];
        break;
    case RV_AND:
        st->regs[rd] = st->regs[rs1] & st->regs[rs2];
        break;
    case RV_FENCE:
        break;
    case RV_ECALL:
        if (st->funcs.ecall(st)) end = 1;
        break;
    case RV_EBREAK:
        st->funcs.ebreak(st);
        break;
    }

    if (!jmp) st->ip += 4;

    return end? RVEXIT_HALT : RVEXIT_SUCCESS;
}

#ifdef RV_USE_DISASM
riscv_exit riscv_disasm(uint32_t inst, char* str, int len)
{
    if (!str || !len) return RVEXIT_ERROR;

    uint32_t imm = 0;
    uint32_t rds[3];
    rds[0] = (inst >> 7) & 0x1F;
    rds[1] = (inst >> 15) & 0x1F;
    rds[2] = (inst >> 20) & 0x1F;
    riscv_op op = decode(inst,&imm);
    if (op > RV_EBREAK) return RVEXIT_WRONGOPCODE;

    int r = snprintf(str,len,"%s ",riscv_names[op]);
    if (r < 0 || r >= len) return RVEXIT_ERROR;
    str += r;
    len -= r;

    for (int i = 0; i < 3; i++)
        if (riscv_useregs[op][i] == '1') {
            r = snprintf(str,len,"%s ",riscv_regname[rds[i]]);
            if (r < 0 || r >= len) return RVEXIT_ERROR;
            str += r;
            len -= r;
        }

    r = snprintf(str,len,"0x%08X",imm);
    if (r < 0 || r >= len) return RVEXIT_ERROR;

    return RVEXIT_SUCCESS;
}
#endif /* RV_USE_DISASM */
