SPATZD_HEADER_TARGETS := $(CAR_SW_DIR)/tests/bare-metal/spatzd/test-snRuntime-simple.h

$(CAR_SW_DIR)/tests/bare-metal/spatzd/test-snRuntime-simple.h: | venv
	cp $(CAR_SW_DIR)/tests/bare-metal/spatzd/build/snRuntime/test-snRuntime-simple $(CAR_SW_DIR)/tests/bare-metal/spatzd/test-snRuntime-simple
	$(VENV)/python $(CAR_ROOT)/scripts/elf2header.py --binary $(CAR_SW_DIR)/tests/bare-metal/spatzd/test-snRuntime-simple --vectors $@
