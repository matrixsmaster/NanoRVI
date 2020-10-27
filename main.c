#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include "riscv.h"

#define NUMREGS 32
#define RAMSIZE (10*1024*1024)

uint32_t regs[NUMREGS] = {0};
uint8_t ram[RAMSIZE] = {0};

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
#if 0
    uint32_t x = in;
    printf("in=0x%08X\n",in);
    for (int i = 0; i < 32; i++,x<<=1) putchar((x&0x80000000)? '1':'0');
    puts("");
#endif
    for (int i = 0; i <= RV_EBREAK; i++) {
//        puts(riscv_encode[i]);
        uint32_t tmp = 0;
        int fnd = 1;
        for (int j = 0; (j < 32) && fnd; j++) {
            if (riscv_encode[i][j] == ' ') continue;
            if (riscv_encode[i][j] >= '<') {
                uint32_t d = (in & (1U << (31-j)))? 1:0;
                d <<= riscv_encode[i][j] - '<';
                tmp |= d;
            } else {
//                printf("eq=0x%08X\n",(in & (1U << (31-j))));
                char d = (in & (1U << (31-j)))? '1':'0';
                if (riscv_encode[i][j] != d) {
#if 0
                    for (int k = 0; k < j; k++) putchar(' ');
                    puts("^");
                    printf("i=%d, pos=%d, expected %c, got %c\n",i,j,riscv_encode[i][j],d);
#endif
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
    uint32_t dbg = x;
    uint32_t c = (x & (1U << bit));
    while (++bit < 32) {
        c <<= 1;
        x |= c;
    }
    printf("[DEBUG] extend: 0x%08x -> 0x%08X (%d)\n",dbg,x,(int)x);
    return (int32_t)x;
}

uint32_t read8(uint32_t addr, int sign)
{
    assert(addr < RAMSIZE);
    uint8_t* ptr = ram + addr;
    return sign? (uint32_t)extend(*ptr,7) : (uint32_t)(*ptr);
}

uint32_t read16(uint32_t addr, int sign)
{
    assert(addr < RAMSIZE);
    uint16_t* ptr = (uint16_t*)(ram + addr);
    return sign? (uint32_t)extend(*ptr,15) : (uint32_t)(*ptr);
}

uint32_t read32(uint32_t addr)
{
    assert(addr < RAMSIZE);
    uint32_t* ptr = (uint32_t*)(ram + addr);
    return *ptr;
}

void write8(uint32_t addr, uint32_t val)
{
    assert(addr < RAMSIZE);
    uint8_t* ptr = ram + addr;
    *ptr = val & 0xFF;
}

void write16(uint32_t addr, uint32_t val)
{
    assert(addr < RAMSIZE);
    uint16_t* ptr = (uint16_t*)(ram + addr);
    *ptr = val & 0xFFFF;
}

void write32(uint32_t addr, uint32_t val)
{
    assert(addr < RAMSIZE);
    uint32_t* ptr = (uint32_t*)(ram + addr);
    *ptr = val;
}

int main(int argc, char* argv[])
{
    assert(argc > 1);

    int ip = readhex(argv[1]);
    assert(ip);

    regs[2] = RAMSIZE - 4;

    while (ip >= 0 && ip < RAMSIZE) {
        assert((ip & 3) == 0);
        regs[0] = 0;

//        assert(ip != 0x10144);

        uint32_t inst = read32(ip);
        uint32_t imm = 0;
        uint32_t rd = (inst >> 7) & 0x1F;
        uint32_t rs1 = (inst >> 15) & 0x1F;
        uint32_t rs2 = (inst >> 20) & 0x1F;
        riscv_op op = decode(inst,&imm);
        assert(op <= RV_EBREAK);

//        if (riscv_useregs[op][0] == '1') assert(rd < NUMREGS);
//        if (riscv_useregs[op][1] == '1') assert(rs1 < NUMREGS);
//        if (riscv_useregs[op][2] == '1') assert(rs2 < NUMREGS);

        printf("0x%08X: %s %s %s %s (0x%06X) SP=%d (0x%08X)\n",ip,riscv_names[op],riscv_regname[rd],riscv_regname[rs1],riscv_regname[rs2],imm,regs[2],regs[2]);
        for (int k = 1; k < 32; k++) printf("%d ",regs[k]);
        puts("");
//        getchar();

        uint32_t jmp = 0;
        switch (op) {
        case RV_LUI:
            regs[rd] = imm;
            break;
        case RV_AUIPC:
            regs[rd] = ip + imm;
            break;
        case RV_JAL:
            regs[rd] = ip + 4;
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
            regs[rd] = regs[rs1] >> rs2 | (regs[rs1] & 0x80000000);
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
            printf("Syscall request encountered at ip=0x%08X\n",ip);
            break;
        case RV_EBREAK:
            printf("Breakpoint encountered at ip=0x%08X\n",ip);
            break;
        }

        if (!jmp) ip += 4;
    }
}
