SPATZD_HEADER_TARGETS := $(CAR_SW_DIR)/tests/bare-metal/spatzd/libomptarget_device.h

$(CAR_SW_DIR)/tests/bare-metal/spatzd/libomptarget_device.h: $(SPATZD_ROOT)/sw/hero/device/apps/libomptarget_device/build/omptarget_device.elf | venv
	$(VENV)/python $(CAR_ROOT)/scripts/elf2header.py --binary $< --vectors $@
