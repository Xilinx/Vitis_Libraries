ENABLE_P2P := 1

ifneq ($(findstring u2x4, $(DEVICE)), u2x4)
	ENABLE_P2P := 0
endif

CXXFLAGS += -DENABLE_P2P=$(ENABLE_P2P)
