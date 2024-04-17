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
#include "printf.h"

extern int load_spatzd_payload ();

int main(void)
{
	volatile uintptr_t mbox_addr = (uintptr_t)CAR_MBOX_BASE_ADDR;
	writew(1, mbox_addr + 0x208);
	writew(1, mbox_addr + 0x248);
	writew(1, mbox_addr + 0x308);
	writew(1, mbox_addr + 0x348);
	car_reset_domain(CAR_SPATZ_RST);
	// Init the HW
	// Safety Island
	car_enable_domain(CAR_SPATZ_RST);

	// Here we assume that the offloader has to poll a status register to catch the end of
	// computation of the Safety Island. Therefore, the offloading is blocking.
	//printf("Starting offloading\n\r");
	uint32_t ret = spatzd_offloader_blocking();

	return ret;
}
