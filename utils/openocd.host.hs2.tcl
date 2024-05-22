# Copyright 2024 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Use this OpenOCD script to connect to debug Carfield via a Digilent
# HS2 USB-JTAG Dongle connected to your FPGA
# Attention: This config requires
# Safety-Island : Disabled
# OpenTitan     : Disabled

adapter_khz 2000
interface ftdi
ftdi_vid_pid 0x0403 0x6014
ftdi_layout_init 0x00e8 0x60eb
ftdi_channel 0
set irlen 5

transport select jtag
telnet_port disabled
tcl_port disabled
reset_config none

set _CHIPNAME carfield
jtag newtap $_CHIPNAME cheshire -irlen ${irlen} -expected-id 0xed9c5e51

set _TARGETNAME $_CHIPNAME.cpu
target create $_TARGETNAME riscv -chain-position $_TARGETNAME -coreid 0

gdb_report_data_abort enable
gdb_report_register_access_error enable

riscv set_reset_timeout_sec 120
riscv set_command_timeout_sec 120

riscv set_prefer_sba off

# Try enabling address translation (only works for newer versions)
if { [catch {riscv set_enable_virtual on} ] } {
    echo "Warning: This version of OpenOCD does not support address translation. To debug on virtual addresses, please update to the latest version." }

init
halt
echo "Ready for Remote Connections"

