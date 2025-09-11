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
#include "car_util.h"

#include "car_iommu.h"

extern void set_iommu_bare();

int main(void)
{
	rv_iommu_cq_init();

	// Setup transparent (but translated) single level for the L2
	s1pt_map(0x78000000, 0x78000000, 1*PAGE_SIZE);

	// Set IOMMU to bare
	set_iommu_1lvl();

	// Init device directory table for device 0
	root_ddt[0].tc = DC_TC_VALID;
	root_ddt[0].iohgatp = 0;
	root_ddt[0].ta = 0;
	root_ddt[0].fsc = (((uintptr_t)&(s1pt[0][0])) >> 12) | (IOSATP_MODE_SV39);

	// Ungate the cluster
	car_enable_domain(CAR_SPATZ_RST);

	// Here we assume that the offloader has to poll a status register to catch the end of
	// computation of the Safety Island. Therefore, the offloading is blocking.
	uint32_t ret = snitchd_offloader_blocking();

	return ret;
}
