# Nano RISC-V 32i emulator
# Copyright (C) Dmitry 'MatrixS_Master' Solovyev, 2020
#
# This work is licensed under the MIT License. See included LICENSE.TXT.

APP = nano_rvi
CC = gcc
LD = gcc

OBJS = main.o
CCFLAGS = -Wall -Wextra -g -O0
LDFLAGS = -Wall -g -Wl,-gc-sections

all: $(OBJS)
	$(LD) $(LDFLAGS) -o $(APP) $(OBJS)
.PHONY: all

clean:
	rm -vf $(OBJS)
	rm -vf $(APP)
.PHONY: clean

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@
