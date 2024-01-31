#Adding extra parameters
ifneq (,$(findstring VCD,$(PARAMS)))
	AIESIMFLAGS += --dump-vcd ${UUT_KERNEL}_sim --simulation-cycle-timeout=100000
endif	
