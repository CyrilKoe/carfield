SPATZD_HEADER_TARGETS := $(CAR_SW_DIR)/tests/bare-metal/snitchd/bin_header/raja_sort.h

$(CAR_SW_DIR)/tests/bare-metal/snitchd/bin_header/raja_sort.h: | venv
	$(VENV)/python $(CAR_ROOT)/scripts/elf2header.py --binary $(CAR_SW_DIR)/tests/bare-metal/snitchd/bin/raja_sort.elf --vectors $@
