#!/bin/bash

# A quick and dirty conversion of RISC-V test battery
# This file (C) Dmitry Solovyev, 2020

cat enum.txt | sort | awk '{ print tolower($1) }' | while read i ; do
    echo "Testing $i..."
    riscv32-unknown-elf-gcc -march=rv32i -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles "$i.S" || exit 1
    R=`riscv_test a.out s | grep "Exiting with code" | awk '{ print $4 }'`
    if [ "z$R" = "z0" ]; then
        echo "SUCCESS"
    else
        echo "FAILURE"
        riscv_test a.out sltr
        exit 3
    fi
done

echo "All tests finished successfully"