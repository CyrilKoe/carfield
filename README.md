# Carfield

This repository hosts the Carfield SoC platform, a mixed-criticality SoC
targeting the automotive domain. It uses
[`Cheshire`](https://github.com/pulp-platform/cheshire) as main host domain. It
is developed as part of the PULP project, a joint effort between ETH Zurich and
the University of Bologna.

## Disclaimer

This project is still considered to be in early development; some parts may not
yet be functional, and existing interfaces and conventions may be broken without
prior notice. We target a formal release in the very near future.

## Dependencies
To handle project dependencies, you can use
[bender](https://github.com/pulp-platform/bender).

## Carfield Initialization
To initialize Carfield, do the following:
* Execute the command:

   ```
   make car-init
   ```

   It will take care of:

   1. Clone all the Carfield dependencies;
   2. Initialize the [Cheshire SoC](https://github.com/pulp-platform/cheshire). This can be
	  done separately by running `make chs-init`
   3. Downloads the Hyperram models from the iis-gitlab. If you don't have access to it, you
	  can also download the freely-available Hyperram models from
	  [here](https://www.cypress.com/documentation/models/verilog/s27kl0641-s27ks0641-verilog)
	  and unzip them according to the bender.

* Check that you have a RISCV toolchain for both RV64 and RV32 ISAs. For ETH, type:
   ```
   source scripts/env-iis.sh
   ```

## Simulation

Follow these steps to launch a Carfield simulation:

### Compile HW and SW

* Generate the compile scripts for Questasim and compile Carfield.

   ```
   make car-hw-build
   ```

  It is also possible to run `make -B scripts/carfield_compile.tcl` to
  re-generate the compile script after hardware modfications.

* Compile tests for Carfield. Tests resides in `sw/tests`.

  ```
  // Compile Safety Island standalone software
  source ./scripts/safed-env.sh
  make safed-sw-build
  // Compile Integer cluster standalone software
  source ./scripts/pulpd-env.sh
  make pulpd-sw-build
  // Compile Cheshire SW
  make car-sw-build
  ```

  The latter commands:
  * Compiles safety island and pulp cluster standalone tests
  * Compiles CVA6 standalone and offloading tests

### System bootmodes

* The current supported bootmodes from Cheshire are:

  | `CHS_BOOTMODE` | `CHS_PRELMODE` | Action |
  | --- | --- | --- |
  | 0 | 0 | Passive bootmode, JTAG preload |
  | 0 | 1 | Passive bootmode, Serial Link preload |
  | 0 | 2 | Passive bootmode, UART preload |
  | 0 | 3 | Passive bootmode, Secure Boot from SECD |
  | 1 | - | Autonomous bootmode, SPI SD card |
  | 2 | - | Autonomous bootmode, SPI flash |
  | 3 | - | Autonomous bootmode, I2C EEPROM |

  `Bootmode` indicates the available bootmodes in Cheshire, while `Preload mode` indicates the type
  of preload, if any is needed. For RTL simulation, bootmodes 0, 2 and 3 are supported. SPI SD card
  bootmode is supported on FPGA emulation.

* The current supported bootmodes for the Safety Island are:

  | `SAFED_BOOTMODE` | Action |
  | --- | --- |
  | 0 | Passive bootmode, JTAG preload |
  | 1 | Passive bootmode, Serial Link preload |

### Simulation

To launch an RTL simulation with the selected boot/preload modes for the island of choice, type:


* For cheshire in passive bootmode (`CHS_BOOTMODE=0`), set `CHS_BINARY` for Cheshire

```
make car-hw-sim CHS_BOOTMODE=<chs_bootmode> CHS_PRELMODE=<chs_prelmode> CHS_BINARY=<chs_binary_path>.car.elf PULPCL_BINARY=<pulpcl_binary> SPATZCL_BINARY=<spatzcl_binary> SECD_BINARY=<secd_binary_path> SAFED_BOOTMODE=<safed_bootmode> SAFED_BINARY=<safed_binary_path>
```

* For cheshire in autonomous bootmode (`CHS_BOOTMODE` = {1,2,3}), set `CHS_IMAGE` for Cheshire

```
make car-hw-sim CHS_BOOTMODE=<chs_bootmode> CHS_PRELMODE=<chs_prelmode> CHS_IMAGE=<chs_binary_path>.car.memh PULPCL_BINARY=<pulpcl_binary> SPATZCL_BINARY=<spatzcl_binary> SECD_BINARY=<secd_binary_path> SAFED_BOOTMODE=<safed_bootmode> SAFED_BINARY=<safed_binary_path>
```

## License

Unless specified otherwise in the respective file headers, all code checked into
this repository is made available under a permissive license. All hardware
sources and tool scripts are licensed under the Solderpad Hardware License 0.51
(see `LICENSE`) with the exception of generated register file code (e.g.
`hw/regs/*.sv`), which is generated by a fork of lowRISC's
[`regtool`](https://github.com/lowRISC/opentitan/blob/master/util/regtool.py)
and licensed under Apache 2.0. All software sources are licensed under Apache
2.0.
