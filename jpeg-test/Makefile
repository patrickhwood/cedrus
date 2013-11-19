include config.mk

CFLAGS += -I../common -MD -MP -MQ $@
MAKEFLAGS += -rR --no-print-directory

OBJ = $(addsuffix .o,$(basename $(SRC)))
LIBOBJ = $(addsuffix .o,$(basename $(LIBSRC)))

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

all: $(LIB) $(TARGET)

$(LIB): $(LIBOBJ)
	@echo "AR $(LIB)"
	ar rf $(LIB) $(LIBOBJ)

$(TARGET): $(OBJ)
	@echo "LD $(TARGET)"
	$(Q)$(GCC) $(LDFLAGS) $(OBJ) -o $@

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
	$(Q)$(GCC) $(CFLAGS) -c $< -o $@

include $(wildcard $(foreach f,$(OBJ),$(basename $(f)).d))
