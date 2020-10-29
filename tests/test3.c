#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define SIZE (1024*10)

int main()
{
    uint32_t crc = 0;
    char* test = (char*)malloc(SIZE); //1MB
    if (!test) {
        puts("Unable to allocate memory!");
        return 1;
    }

    printf("test: %p; test[0] = %hhu\n",test,test[0]);

    for (int i = 0; i < SIZE; i++) {
        crc += (char)i;
        test[i] = (char)i;
        if (test[i] != (char)i) printf("ERROR @ %d: expected 0x%02X, got 0x%02X\n",i,(char)i,test[i]);
        //__asm ( "EBREAK" );
    }

    printf("Original CRC = %u\n",crc);

    crc = 0;
    for (int i = 0; i < SIZE; i++) {
        if (test[i] != (char)i) printf("ERROR @ %d: expected 0x%02X, got 0x%02X\n",i,(char)i,test[i]);
        crc += test[i];
    }

    printf("New CRC = %u\n",crc);

    free(test);

    return 0;
}