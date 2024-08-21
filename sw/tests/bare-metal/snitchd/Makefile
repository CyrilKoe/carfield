
RISCV_PREFIX  ?=
RISCV_CC      ?= clang
RISCV_OBJDUMP ?= llvm-objdump
RISCV_OBJCOPY ?= llvm-objcopy
RISCV_AR      ?= llvm-ar
RISCV_AS      ?= llvm-as

RISCV_FLAGS    ?= -g -O2 -mcpu=snitch -mabi=ilp32d -march=rv32imafd_zfh1p0_xfrep0p1_xssr0p1_xdma0p1_xfalthalf0p1_xfquarter0p1_xfaltquarter0p1_xfvecsingle0p1_xfvechalf0p1_xfvecalthalf0p1_xfvecquarter0p1_xfvecaltquarter0p1_xfauxhalf0p1_xfauxalthalf0p1_xfauxquarter0p1_xfauxaltquarter0p1_xfauxvecsingle0p1_xfauxvechalf0p1_xfauxvecalthalf0p1_xfauxvecquarter0p1_xfauxvecaltquarter0p1_xfexpauxvechalf0p1_xfexpauxvecalthalf0p1_xfexpauxvecquarter0p1_xfexpauxvecaltquarter0p1 -ffreestanding --sysroot=$(HERO_INSTALL)/rv32imafd-ilp32d/riscv32-unknown-elf
RISCV_LDFLAGS  ?= -static -nostdlib -L$(HERO_INSTALL)/lib/clang/15.0.0/rv32imafd-ilp32d/lib/ -lclang_rt.builtins-riscv32
RISCV_CCFLAGS  ?= $(RISCV_FLAGS) -DPRINTF_DISABLE_SUPPORT_EXPONENTIAL -Idevice/utils -Icommon -Idevice/runtime

HOST_RISCV_FLAGS    ?= -target riscv64-unknown-elf -fPIC -mabi=lp64d -g -O0 -march=rv64imafd -Icommon -ffreestanding --sysroot=$(HERO_INSTALL)/rv64imafd-lp64d/riscv64-unknown-elf -mcmodel=medany -mno-relax
HOST_RISCV_LDFLAGS  ?= -static -nostdlib $(HOST_RISCV_FLAGS)
HOST_RISCV_CCFLAGS  ?= $(HOST_RISCV_FLAGS)

N ?= 16
BUF_SIZE ?= 8

DEFINES_raja_sort ?= -DVEC_SIZE=$(N) -DBUF_SIZE=$(BUF_SIZE)

all:

bin/%.elf bin/%.dump bin/%.bin: device/runtime/link.ld device/runtime/crt0.S device/%.c # common/printf.c common/sw_mailbox.c
	$(RISCV_CC) $(RISCV_CCFLAGS) $(DEFINES_$*) -o bin/$*.elf $(RISCV_LDFLAGS) -T $^
	$(RISCV_OBJDUMP) -S -D bin/$*.elf > bin/$*.dump
	$(RISCV_OBJCOPY) -O binary bin/$*.elf bin/$*.bin --only-section=.text --only-section=.data --only-section=.sdata --only-section=.rodata

bin/host_%.elf bin/host_%.dump bin/host_%.bin: bin/%.elf host/link.ld host/crt0.S host/main.c host/uart.c common/printf.c common/sw_mailbox.c host/%.c
	$(RISCV_CC) $(HOST_RISCV_CCFLAGS) $(DEFINES_$*) -o bin/host_$*.elf $(HOST_RISCV_LDFLAGS) -DDEVICEBIN=\"bin/$*.bin\" -T $(filter-out %.elf,$^)
	$(RISCV_OBJDUMP) -S -D bin/host_$*.elf > bin/host_$*.dump
	$(RISCV_OBJCOPY) -O binary bin/host_$*.elf bin/host_$*.bin --only-section=.text --only-section=.data --only-section=.devicebin

bin/libomptarget_device.a: device/runtime/link.ld device/runtime/crt0.S device/omptarget.c common/printf.c common/sw_mailbox.c
	$(RISCV_CC) -c $(RISCV_CCFLAGS) $(DEFINES_$*) device/runtime/crt0.S -o bin/crt0.o
	$(RISCV_CC) -c $(RISCV_CCFLAGS) $(DEFINES_$*) common/printf.c -o bin/printf.o
	$(RISCV_CC) -c $(RISCV_CCFLAGS) $(DEFINES_$*) common/sw_mailbox.c -o bin/sw_mailbox.o
	$(RISCV_CC) -c $(RISCV_CCFLAGS) $(DEFINES_$*) device/omptarget.c -o bin/omptarget.o
	llvm-ar rcs $@ bin/crt0.o bin/printf.o bin/sw_mailbox.o bin/omptarget.o
	$(RISCV_OBJDUMP) -S -D $@ > bin/libomptarget_device.dump


clean:
	rm bin/*
.PHONY: clean

.PRECIOUS: bin/*