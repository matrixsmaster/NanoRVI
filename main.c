#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include "riscv.h"

#define RAMSIZE (10*1024*1024)

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

int main(int argc, char* argv[])
{
    assert(argc > 1);

    int ip = readhex(argv[1]);
    assert(ip);

    while (ip >= 0 && ip < RAMSIZE) {
        uint32_t inst = 0;
        memcpy(&inst,ram+ip,4);
        uint32_t imm = 0;
        uint32_t rd = (inst >> 7) & 0x1F;
        uint32_t rs1 = (inst >> 15) & 0x1F;
        uint32_t rs2 = (inst >> 20) & 0x1F;
        riscv_op op = decode(inst,&imm);
        assert(op <= RV_EBREAK);

        printf("0x%08X: %s %d %d %d %d (0x%06X)\n",ip,riscv_names[op],rd,rs1,rs2,(int)imm,imm);
        ip += 4; ///fixme: debug only
    }
}
