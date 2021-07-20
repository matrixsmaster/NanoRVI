/*
 *
 * Nano RISC-V 32i emulator
 * Copyright (C) Dmitry 'MatrixS_Master' Solovyev, 2020-2021
 *
 * This work is licensed under the MIT License. See included LICENSE file
 *
 * */

#ifndef RISCV_H_
#define RISCV_H_

#include <inttypes.h>

#define RV_NUMREGS 32

#define RV_ENCODE_SYM_DONT_CARE ' '
#define RV_ENCODE_SYM_IMM_START '<'

#define RV_USE_DISASM

// I made this to make sign extend easily optimizable by a compiler - should be converted into 3 or 4 instructions
#define RV_EXTEND(X,B) ((int32_t)( ((X) & (1U << (B))) ? ((X) | (((1U << (32 - ((B)+1))) - 1) << ((B)+1))) : (X) ))

typedef enum {
    RV_LUI,
    RV_AUIPC,
    RV_JAL,
    RV_JALR,
    RV_BEQ,
    RV_BNE,
    RV_BLT,
    RV_BGE,
    RV_BLTU,
    RV_BGEU,
    RV_LB,
    RV_LH,
    RV_LW,
    RV_LBU,
    RV_LHU,
    RV_SB,
    RV_SH,
    RV_SW,
    RV_ADDI,
    RV_SLTI,
    RV_SLTIU,
    RV_XORI,
    RV_ORI,
    RV_ANDI,
    RV_SLLI,
    RV_SRLI,
    RV_SRAI,
    RV_ADD,
    RV_SUB,
    RV_SLL,
    RV_SLT,
    RV_SLTU,
    RV_XOR,
    RV_SRL,
    RV_SRA,
    RV_OR,
    RV_AND,
    RV_FENCE,
    RV_ECALL,
    RV_EBREAK
} riscv_op;

typedef enum {
    RVR_ZERO,
    RVR_RA,
    RVR_SP,
    RVR_GP,
    RVR_TP,
    RVR_T0,
    RVR_T1,
    RVR_T2,
    RVR_S0,
    RVR_S1,
    RVR_A0,
    RVR_A1,
    RVR_A2,
    RVR_A3,
    RVR_A4,
    RVR_A5,
    RVR_A6,
    RVR_A7,
    RVR_S2,
    RVR_S3,
    RVR_S4,
    RVR_S5,
    RVR_S6,
    RVR_S7,
    RVR_S8,
    RVR_S9,
    RVR_S10,
    RVR_S11,
    RVR_T3,
    RVR_T4,
    RVR_T5,
    RVR_T6
} riscv_reg;

// Execution end result codes
typedef enum {
    RVEXIT_SUCCESS = 0,
    RVEXIT_HALT,
    RVEXIT_ERROR,
    RVEXIT_WRONGOPCODE,
} riscv_exit;

// Callbacks, conveniently brought together
typedef struct riscv_state_s riscv_state; // just a forward decl.
typedef struct {
    uint32_t  (*read8)(riscv_state* state, uint32_t addr);
    uint32_t (*read16)(riscv_state* state, uint32_t addr);
    uint32_t (*read32)(riscv_state* state, uint32_t addr);

    void  (*write8)(riscv_state* state, uint32_t addr, uint32_t val);
    void (*write16)(riscv_state* state, uint32_t addr, uint32_t val);
    void (*write32)(riscv_state* state, uint32_t addr, uint32_t val);

    uint8_t (*ecall)(riscv_state* state);
    void (*ebreak)(riscv_state* state);
} riscv_callbacks;

// Virtual machine state main structure
typedef struct riscv_state_s {
    uint32_t ip;                /* The Instruction Pointer */
    uint32_t regs[RV_NUMREGS];  /* CPU Registers */
    riscv_callbacks funcs;      /* Interface callback functions */
    void* user;                 /* User-defined data */
} riscv_state;

// Main function
riscv_exit riscv_exec(riscv_state* st);

#ifdef RV_USE_DISASM
// Helper function - disassemble single operation
riscv_exit riscv_disasm(uint32_t inst, char* str, int len);
#endif /* RV_USE_DISASM */

#endif /* RISCV_H_ */
