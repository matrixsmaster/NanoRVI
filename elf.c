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
#include "elf.h"
#include "riscv.h"
#include "debug.h"

static bool readelf_internal(rv_interface* vm, FILE* fin)
{
    // get ELF header
    elf_header_t elfhdr;
    if (!fread(&elfhdr,sizeof(elfhdr),1,fin)) return false;

    // check that it's of correct version and machine type
    const char magic[] = "\x7f" "ELF";
    if (memcmp(magic,elfhdr.magic,4)) return false;
    if (elfhdr.ver != 1) return false;
    if (elfhdr.class != 1) return false;
    if (elfhdr.endianness != 1) return false;
    if (elfhdr.machine != ELF_RISCV_MACH_CODE) return false;

    // jump into program headers section
    fseek(fin,elfhdr.proghdr_off,SEEK_SET);

    // for each header block
    for (uint16_t i = 0; i < elfhdr.proghdr_num; i++) {
        // get data
        elf_proghdr_t phdr;
        if (!fread(&phdr,sizeof(phdr),1,fin)) return false;

        // would it fit into our RAM ?
        vm->prog_break = phdr.vaddr + phdr.memsz;
        if (vm->prog_break >= vm->ram_size) {
            printf("ERROR: ELF Section %u is too big (0x%08X - 0x%08X) to fit in RAM\n",i,phdr.vaddr,vm->prog_break);
            return false;
        }

        // jump to start of the actual data referenced in this section
        size_t pos = ftell(fin);
        fseek(fin,phdr.off,SEEK_SET);

        // and load it all up
        if (!fread(vm->ram+phdr.vaddr,phdr.filesz,1,fin)) return false;
        fseek(fin,pos,SEEK_SET);

        if (vm->debug & DBG_LOAD)
            printf("Section %u loaded, 0x%08X - 0x%08X, %u bytes\n",i,phdr.vaddr,vm->prog_break,phdr.memsz);
    }

    vm->start = elfhdr.entry;
    return true;
}

// Load and parse an ELF file, and put it properly into RAM
bool readelf(rv_interface* vm, const char* fn)
{
    FILE* f = fopen(fn,"rb");
    if (!f) {
        printf("ERROR: Unable to open file '%s'\n", fn);
        return false;
    }

    bool ret = readelf_internal(vm,f);
    fclose(f);

    if (!ret) printf("ERROR: Unable to parse ELF file\n");

    return ret;
}
