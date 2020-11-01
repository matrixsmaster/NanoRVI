/*
 *
 * Nano RISC-V 32i emulator
 * Copyright (C) Dmitry 'MatrixS_Master' Solovyev, 2020
 *
 * This work is licensed under the MIT License. See included LICENSE file
 *
 * */

#ifndef ELF_H_
#define ELF_H_

typedef struct {
    char magic[4];
    uint8_t class;
    uint8_t endianness;
    uint8_t ver;
    uint8_t abi;
    uint8_t abiver;
    char pad[7];
    uint16_t type;
    uint16_t machine;
    uint32_t ver_again;
    uint32_t entry;
    uint32_t proghdr_off;
    uint32_t secthdr_off;
    uint32_t flags;
    uint16_t hdrsize;
    uint16_t proghdr_size;
    uint16_t proghdr_num;
    uint16_t secthdr_size;
    uint16_t secthdr_num;
    uint16_t name_idx;
} elf_header_t;

typedef struct {
    uint32_t type;
    uint32_t off;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
} elf_proghdr_t;

#endif /* ELF_H_ */
