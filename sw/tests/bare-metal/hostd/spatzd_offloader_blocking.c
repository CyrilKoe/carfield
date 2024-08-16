// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Alessandro Ottaviano <aottaviano@iis.ee.ethz.ch>
//

#include <stdio.h>
#include <stdint.h>

#include "car_memory_map.h"
#include "io.h"
#include "dif/clint.h"
#include "dif/uart.h"
#include "params.h"
#include "regs/cheshire.h"
#include "util.h"
#include "payload.h"
#include "car_util.h"
//#include "printf.h"

/*
#define BIT_MASK(pos, len) ((1<<(len))-1 << (pos))
#include "rv_iommu.h"
#include "page_tables.h"

extern int load_spatzd_payload ();

#define PAGE_SIZE           0x1000ULL     // 4kiB
extern ddt_t root_ddt[DDT_N_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
extern pte_t s1pt[6][PAGE_SIZE/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE)));
*/ 
#if 0
//typedef uint64_t pte_t;
//pte_t s1pt[6][PAGE_SIZE/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE)));
#define PT_SIZE (PAGE_SIZE)
#define PAGE_ADDR_MSK (~(PAGE_SIZE - 1))  // ... 1111 1111 0000 0000 0000
#define PAGE_SHIFT (12)

// N = 0:   1 GiB superpages (addr[55:30] = '0) (0x40000000)
// N = 1:   2 MiB superpages (addr[55:21] = '0) (0x00200000)
// N = 2:   4 kiB pages      (addr[55:12] = '0) (0x00001000)
#define SUPERPAGE_SIZE(N) ((PAGE_SIZE) << (((2-N))*9))

#define PT_LVLS (3)  // assumes sv39 for rv64
#define PTE_INDEX_SHIFT(LEVEL) ((9 * (PT_LVLS - 1 - (LEVEL))) + 12)
#define PTE_ADDR_MSK BIT_MASK(12, 44)

#define PTE_INDEX(LEVEL, ADDR) (((ADDR) >> PTE_INDEX_SHIFT(LEVEL)) & (0x1FF))
#define PTE_FLAGS_MSK BIT_MASK(0, 8)

#define PTE_VALID   (1ULL << 0)
#define PTE_READ    (1ULL << 1)
#define PTE_WRITE   (1ULL << 2)
#define PTE_EXECUTE (1ULL << 3)
#define PTE_USER    (1ULL << 4)
#define PTE_GLOBAL  (1ULL << 5)
#define PTE_ACCESS  (1ULL << 6)
#define PTE_DIRTY   (1ULL << 7)

#define PTE_V       PTE_VALID
#define PTE_AD      (PTE_ACCESS | PTE_DIRTY)
#define PTE_U       PTE_USER
#define PTE_R       (PTE_READ)
#define PTE_RW      (PTE_READ | PTE_WRITE)
#define PTE_X       (PTE_EXECUTE)
#define PTE_RX      (PTE_READ | PTE_EXECUTE)
#define PTE_RWX     (PTE_READ | PTE_WRITE | PTE_EXECUTE)

#define PTE_PPN_MSK (0x3FFFFFFFFFFC00ULL)
#endif


/*
void s1pt_map(uint64_t p_addr, uint64_t iov_addr, uint64_t size) {
	uint64_t n_pages = size >> 10;
	for(uint64_t i = 0; i < n_pages; i++) {
		s1pt[2][PTE_INDEX(2, iov_addr)] = (p_addr >> 2) & PTE_ADDR_MSK | PTE_AD | PTE_V | PTE_U | PTE_RWX;
		s1pt[1][PTE_INDEX(1, iov_addr)] = ((uint64_t)&(s1pt[2][PTE_INDEX(2, iov_addr)])) >> 2 & PTE_ADDR_MSK | PTE_V;
		s1pt[0][PTE_INDEX(0, iov_addr)] = ((uint64_t)&(s1pt[1][PTE_INDEX(1, iov_addr)])) >> 2 & PTE_ADDR_MSK | PTE_V;
		//printf("s1pt[2][%u] (%p) = %llx\n\r", PTE_INDEX(2, iov_addr), &s1pt[2][PTE_INDEX(2, iov_addr)], (p_addr >> 2) & PTE_ADDR_MSK | PTE_AD | PTE_V | PTE_U | PTE_RWX);
		//printf("s1pt[1][%u] (%p) = %llx\n\r", PTE_INDEX(1, iov_addr), &s1pt[1][PTE_INDEX(1, iov_addr)], ((uint64_t)&(s1pt[2][PTE_INDEX(2, iov_addr)])) >> 2 & PTE_ADDR_MSK | PTE_V);
		//printf("s1pt[0][%u] (%p) = %llx\n\r", PTE_INDEX(0, iov_addr), &s1pt[0][PTE_INDEX(0, iov_addr)], ((uint64_t)&(s1pt[1][PTE_INDEX(1, iov_addr)])) >> 2 & PTE_ADDR_MSK | PTE_V);

		p_addr += PAGE_SIZE;
		iov_addr += PAGE_SIZE;
	}
}
*/

int main(void)
{
    //car_init_start();
    //uint32_t rtc_freq = *reg32(&__base_regs, CHESHIRE_RTC_FREQ_REG_OFFSET);
	//uint64_t reset_freq = clint_get_core_freq(rtc_freq, 2500);
	//uart_init(&__base_uart, reset_freq, 115200);
	//printf("s1pt %p\n\r", &(s1pt[0][0]));

	// Setup transparent (but translated) single level for the L2
	//s1pt_map(0x78000000, 0x78000000, 1*PAGE_SIZE);

	// Init control queue
	//rv_iommu_cq_init();

	// Init fault queue
	// rv_iommu_fq_init();

	// Init device directory table for device 0
	//root_ddt[0].tc = test_dc_tc_table[BASIC];
	//root_ddt[0].iohgatp = 0;
	//root_ddt[0].ta = 0;
	//root_ddt[0].fsc = (((uintptr_t)&(s1pt[0][0])) >> 12) | (IOSATP_MODE_SV39);

	// Start IOMMU
	//set_iommu_1lvl();

	// Ungate the cluster
	car_enable_domain(CAR_SPATZ_RST);

	// Here we assume that the offloader has to poll a status register to catch the end of
	// computation of the Safety Island. Therefore, the offloading is blocking.
	uint32_t ret = spatzd_offloader_blocking();

	return ret;
}
