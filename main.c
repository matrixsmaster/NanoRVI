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
#include "riscv.h"
#include "interface.h"
#include "debug.h"
#include "elf.h"

// Helper function to print out nicely formatted usage instructions
static void usage(const char* progname)
{
    printf("Usage: %s <command> <argument> ... [<command> <argument>]\n",progname);
    printf("\nAvailable commands are:\n");
    printf("\t-m: set the amount of virtual RAM available (in KiB)\n");
    printf("\t-s: set the size of the stack (in KiB)\n");
    printf("\t-f: execute ELF file\n");
    printf("\t-d: set debug options (string of characters, see below)\n");
    printf("\nAvailable debug options are:\n");
    printf("\tt - enable trace output\n");
    printf("\ts - verbose syscalls\n");
    printf("\tm - trace all memory transactions\n");
    printf("\tr - print registers contents in trace output\n");
    printf("\ti - enable interactive, step-by-step mode\n");
    printf("\tl - verbose program loading procedure\n");
}

// Helper function to read command line arguments
static bool read_args(rv_interface* iface, int argc, char* argv[])
{
    int fsm = 0;
    int loaded = 0;
    for (int i = 1; i < argc; i++) {
        switch (fsm) {
        case 0:
            if (argv[i][0] != '-' || !argv[i][1]) {
                printf("ERROR: malformed argument %d\n",i);
                return false;
            }

            switch (argv[i][1]) {
            case 'm': fsm = 1; break;
            case 's': fsm = 2; break;
            case 'f': fsm = 3; break;
            case 'd': fsm = 4; break;
            default:
                printf("ERROR: Unknown command switch '%c'\n",argv[i][1]);
                return false;
            }
            break;

        case 1: // RAM size
            iface->ram_size = atol(argv[i]) * 1024;
            fsm = 0;
            break;

        case 2: // Stack size
            iface->stack_size = atol(argv[i]) * 1024;
            fsm = 0;
            break;

        case 3: // ELF file
            if (!readelf(iface,argv[i])) return false;
            loaded = 1;
            fsm = 0;
            break;

        case 4: // Debug options
            iface->debug = debug_readopts(argv[i]);
            fsm = 0;
            break;

        default:
            fsm = 0;
        }
    }

    return (iface->ram_size && iface->stack_size && loaded);
}

// Emulator entry point :)
int main(int argc, char* argv[])
{
    // Only one virtual CPU for now
    rv_interface iface;
    rv_iface_init(&iface);

    // Read command line arguments, filling our interface structure
    if (!read_args(&iface,argc,argv)) {
        usage(argv[0]);
        return 1;
    }

    // Check that we can prepare VM for execution
    if (!rv_iface_start(&iface)) {
        printf("ERROR: unable to start virtual machine\n");
        return 2;
    }

    // Execute it until finished (or until the thermal death of the Universe)
    while (rv_iface_step(&iface)) ;

    // Finally, we're done. Let's free up some resources
    rv_iface_stop(&iface);

    // To make it clear we haven't crashed, but exited peacefully...
    puts("Quit");
    return 0;
}
