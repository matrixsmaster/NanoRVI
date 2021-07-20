# Nano RISC-V 32i emulator
# Copyright (C) Dmitry 'MatrixS_Master' Solovyev, 2020-2021
#
# This work is licensed under the MIT License. See included LICENSE file

CC = gcc
LD = gcc

APP = nano_rvi
OBJS = main.o riscv.o interface.o debug.o elf.o sdl_wrapper.o

.PHONY: all
all: OPTIONS = -O0 -g -DDEBUG=1
all: $(APP)

.PHONY: release
release: OPTIONS = -O2
release: $(ROM_HEADERS) $(APP)
	strip $(APP)

CCFLAGS = -Wall -Wextra $(OPTIONS)
LDFLAGS = -Wl,-gc-sections -lSDL2

.PHONY: clean
clean:
	rm -vf $(OBJS)
	rm -vf $(APP)

.PHONY: test
test:
	cd tests/suite && ./do.sh

$(APP): $(OBJS)
	$(LD) $(LDFLAGS) -o $(APP) $(OBJS)

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@
