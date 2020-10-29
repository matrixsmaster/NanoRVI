#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include "riscv.h"

#define NUMREGS 32
#define RAMSIZE (10*1024*1024)
#define STACKSIZE (2*1024*1024)
#define HEAPSIZE (RAMSIZE-STACKSIZE)

enum debug_opts {
    DBG_TRACE = 0x01,
    DBG_SYSCALL = 0x02,
    DBG_MEM = 0x04,
    DBG_REGS = 0x08,
    DBG_INTERACTIVE = 0x10,
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
        { 0, 0 }
};

uint32_t regs[NUMREGS] = {0};
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

int inbyte(char** in)
{
    char tmp[3] = {0};
    tmp[0] = *((*in)++);
    tmp[1] = *((*in)++);
    return strtol(tmp,NULL,16);
}

int readhex(const char* fn)
{
    FILE* fi = fopen(fn,"r");
    if (!fi) return 0;

    char buf[256];
    char* ptr;
    int seg,off = 0;
    int start = 0;
    while (fgets(buf,sizeof(buf),fi)) {
        if (buf[0] != ':') continue;

        ptr = buf + 1;
        int len = inbyte(&ptr);
        int adr = inbyte(&ptr) << 8;
        adr |= inbyte(&ptr);
        int typ = inbyte(&ptr);

        switch (typ) {
        case 0x00:
            for (int i = 0; i < len; i++) {
                assert(off+adr+i < RAMSIZE);
                ram[off+adr+i] = inbyte(&ptr);
            }
            code_end = off+adr+len;
            break;

        case 0x01:
            break;

        case 0x02:
            off = inbyte(&ptr) << 8;
            off |= inbyte(&ptr);
            off <<= 4;
            break;

        case 0x03:
            assert(len == 4);
            seg = inbyte(&ptr) << 8;
            seg |= inbyte(&ptr);
            seg <<= 4;
            off = inbyte(&ptr) << 8;
            off |= inbyte(&ptr);
            start = seg + off;
            break;

        default:
            printf("Unknown record type 0x%02X\n",typ);
            return 0;
        }

        if (typ == 0x01) break;
    }
    fclose(fi);

    return start;
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
    return RV_EBREAK + 1; //invalid op
}

int32_t extend(uint32_t x, int bit)
{
    uint32_t c = (x & (1U << bit));
    while (++bit < 32) {
        c <<= 1;
        x |= c;
    }
    return (int32_t)x;
}

uint32_t read8(uint32_t addr, int sign)
{
    assert(addr < RAMSIZE);
    uint8_t* ptr = ram + addr;
    uint32_t val = sign? (uint32_t)extend(*ptr,7) : (uint32_t)(*ptr);
    if (debug & DBG_MEM) printf("Read %s byte from 0x%08X: 0x%02X\n",(sign? "signed":"unsigned"),addr,val);
    return val;
}

uint32_t read16(uint32_t addr, int sign)
{
    assert(addr < RAMSIZE);
    uint16_t* ptr = (uint16_t*)(ram + addr);
    uint32_t val = sign? (uint32_t)extend(*ptr,15) : (uint32_t)(*ptr);
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
    switch (regs[RVR_A7]) {
    case RVSYS_CLOSE:
        //TODO
        regs[RVR_A0] = 0; // success
        break;

    case RVSYS_WRITE:
        for (int j = 0; j < regs[RVR_A2]; j++) putchar(read8(regs[RVR_A1]+j,0));
        regs[RVR_A0] = regs[RVR_A2]; // return length field
        break;

    case RVSYS_FSTAT:
        //TODO
        regs[RVR_A0] = 0; // success
        break;

    case RVSYS_EXIT:
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

int main(int argc, char* argv[])
{
    assert(argc > 1);

    if (argc > 2) readopts(argv[2]);

    int ip = readhex(argv[1]);
    assert(ip);

    regs[RVR_SP] = RAMSIZE - 4;

    while (ip >= 0 && ip < RAMSIZE) {
        assert((ip & 3) == 0);
        regs[RVR_ZERO] = 0;

        uint32_t inst = read32(ip);
        uint32_t imm = 0;
        uint32_t rd = (inst >> 7) & 0x1F;
        uint32_t rs1 = (inst >> 15) & 0x1F;
        uint32_t rs2 = (inst >> 20) & 0x1F;
        riscv_op op = decode(inst,&imm);
        assert(op <= RV_EBREAK);

        if (debug & DBG_TRACE)
            printf("0x%08X: %s %s %s %s (0x%06X) SP=%d (0x%08X)\n",ip,riscv_names[op],riscv_regname[rd],riscv_regname[rs1],riscv_regname[rs2],imm,regs[2],regs[2]);

        if (debug & DBG_REGS) {
            for (int k = 1; k < 32; k++) printf("%d ",regs[k]);
            puts("");
        }

        if (debug & DBG_INTERACTIVE) getchar();

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
            ip += extend(imm,20);
            jmp = 1;
            break;
        case RV_JALR:
            if (rd) regs[rd] = ip + 4;
            ip = (regs[rs1] + extend(imm,11)) & 0xFFFFFFFE;
            jmp = 1;
            break;
        case RV_BEQ:
            ip += (regs[rs1] == regs[rs2])? extend(imm,12):4;
            jmp = 1;
            break;
        case RV_BNE:
            ip += (regs[rs1] != regs[rs2])? extend(imm,12):4;
            jmp = 1;
            break;
        case RV_BLT:
            ip += ((int32_t)(regs[rs1]) < (int32_t)(regs[rs2]))? extend(imm,12):4;
            jmp = 1;
            break;
        case RV_BGE:
            ip += ((int32_t)(regs[rs1]) >= (int32_t)(regs[rs2]))? extend(imm,12):4;
            jmp = 1;
            break;
        case RV_BLTU:
            ip += (regs[rs1] < regs[rs2])? extend(imm,12):4;
            jmp = 1;
            break;
        case RV_BGEU:
            ip += (regs[rs1] >= regs[rs2])? extend(imm,12):4;
            jmp = 1;
            break;
        case RV_LB:
            regs[rd] = read8(regs[rs1]+extend(imm,11),1);
            break;
        case RV_LH:
            regs[rd] = read16(regs[rs1]+extend(imm,11),1);
            break;
        case RV_LW:
            regs[rd] = read32(regs[rs1]+extend(imm,11));
            break;
        case RV_LBU:
            regs[rd] = read8(regs[rs1]+extend(imm,11),0);
            break;
        case RV_LHU:
            regs[rd] = read16(regs[rs1]+extend(imm,11),0);
            break;
        case RV_SB:
            write8(regs[rs1]+extend(imm,11),regs[rs2]);
            break;
        case RV_SH:
            write16(regs[rs1]+extend(imm,11),regs[rs2]);
            break;
        case RV_SW:
            write32(regs[rs1]+extend(imm,11),regs[rs2]);
            break;
        case RV_ADDI:
            regs[rd] = (uint32_t)((int32_t)(regs[rs1]) + extend(imm,11));
            break;
        case RV_SLTI:
            regs[rd] = ((int32_t)(regs[rs1]) < extend(imm,11))? 1:0;
            break;
        case RV_SLTIU:
            regs[rd] = (regs[rs1] < (uint32_t)(extend(imm,11)))? 1:0;
            break;
        case RV_XORI:
            regs[rd] = regs[rs1] ^ extend(imm,11);
            break;
        case RV_ORI:
            regs[rd] = regs[rs1] | extend(imm,11);
            break;
        case RV_ANDI:
            regs[rd] = regs[rs1] & extend(imm,11);
            break;
        case RV_SLLI:
            regs[rd] = regs[rs1] << rs2;
            break;
        case RV_SRLI:
            regs[rd] = regs[rs1] >> rs2;
            break;
        case RV_SRAI:
            regs[rd] = (regs[rs1] >> rs2) | (regs[rs1] & 0x80000000);
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
            regs[rd] = (regs[rs1] >> (regs[rs2] & 0x1F)) | (regs[rs1] & 0x80000000);
            break;
        case RV_OR:
            regs[rd] = regs[rs1] | regs[rs2];
            break;
        case RV_AND:
            regs[rd] = regs[rs1] & regs[rs2];
            break;
        case RV_FENCE:
            printf("FENCE instruction encountered at ip=0x%08X\n",ip);
            break;
        case RV_ECALL:
            if (debug & DBG_SYSCALL)
                printf("Syscall request %u encountered at ip=0x%08X\n",regs[RVR_A7],ip);
            if (ecall()) ip = RAMSIZE;
            break;
        case RV_EBREAK:
            printf("Breakpoint encountered at ip=0x%08X\nPress Enter to continue\n",ip);
            getchar();
            break;
        }

        if (!jmp) ip += 4;
    }

    puts("Quit");
}
