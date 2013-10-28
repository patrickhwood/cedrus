include config.mk

CFLAGS += -g -O2 -I../common -MD -MP -MQ $@
MAKEFLAGS += -rR --no-print-directory

OBJ = $(addsuffix .o,$(basename $(SRC)))

ifdef V
	ifeq ("$(origin V)", "command line")
		VERBOSE = $(V)
	endif
endif
ifndef VERBOSE
	VERBOSE = 0
endif

ifeq ($(VERBOSE), 1)
	Q =
else
	Q = @
endif

$(TARGET): $(OBJ)
	@echo "LD $(TARGET)"
	$(Q)g++ $(OBJ) $(LDFLAGS) -o $@

.PHONY: clean
clean:
	@echo "RM *.o"
	$(Q)rm -f *.o
	@echo "RM *.d"
	$(Q)rm -f *.d
	@echo "RM $(TARGET)"
	$(Q)rm -f $(TARGET)

%.o: %.c config.mk
	@echo "CC $<"
	$(Q)gcc $(CFLAGS) -c $< -o $@

include $(wildcard $(foreach f,$(OBJ),$(basename $(f)).d))
