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

#define BIT_MASK(pos, len) ((1<<(len))-1 << (pos))
#include "rv_iommu.h"
#include "page_tables.h"

extern int load_spatzd_payload ();

extern ddt_t root_ddt[DDT_N_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
extern pte_t s1pt[6][PAGE_SIZE/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE)));

void s1pt_map(uint64_t p_addr, uint64_t iov_addr, uint64_t size) {
	uint64_t n_pages = size >> 10;
	for(uint64_t i = 0; i < n_pages; i++) {
		s1pt[2][PTE_INDEX(2, p_addr)] = (iov_addr >> 2) & PTE_ADDR_MSK | PTE_AD | PTE_V | PTE_U | PTE_RWX;
		s1pt[1][PTE_INDEX(1, p_addr)] = ((uint64_t)&(s1pt[2][PTE_INDEX(2, p_addr)])) >> 2 & PTE_ADDR_MSK | PTE_V;
		s1pt[0][PTE_INDEX(0, p_addr)] = ((uint64_t)&(s1pt[1][PTE_INDEX(1, p_addr)])) >> 2 & PTE_ADDR_MSK | PTE_V;
		p_addr += PAGE_SIZE;
		iov_addr += PAGE_SIZE;
	}
}

int main(void)
{
	// Setup transparent (but translated) single level for the L2
	s1pt_map(0x78000000, 0x78000000, 128*PAGE_SIZE);

	// Init control queue
	rv_iommu_cq_init();

	// Init fault queue
	// rv_iommu_fq_init();

	// Init device directory table for device 0
	root_ddt[0].tc = test_dc_tc_table[BASIC];
	root_ddt[0].iohgatp = 0;
	root_ddt[0].ta = 0;
	root_ddt[0].fsc = (((uintptr_t)s1pt) >> 12) | (IOSATP_MODE_SV39);

	// Start IOMMU
	set_iommu_1lvl();

	// Wake up the cluster
	car_enable_domain(CAR_SPATZ_RST);

	// Here we assume that the offloader has to poll a status register to catch the end of
	// computation of the Safety Island. Therefore, the offloading is blocking.
	uint32_t ret = spatzd_offloader_blocking();

	return ret;
}
