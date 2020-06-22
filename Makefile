BINB    = backup.e4
BINR    = restore.e4
CC      = gcc
STRIP   = strip

DEBUG  ?= 0
INTEGRATION ?= 0

ifeq ($(DEBUG), 1)
OFLAGS  = -g
else
OFLAGS  = -O3 -flto
endif

ifeq ($(INTEGRATION),0)
ECHO    = @
else
ECHO    =
endif

CFLAGS  = $(OFLAGS) -Wall -fdata-sections -ffunction-sections -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS += -DBINB=$(BINB) -DBINR=$(BINR)
ifeq ($(DEBUG), 0)
CFLAGS += -DNDEBUG
endif
LDFLAGS = $(OFLAGS) -Wl,--gc-sections -lz

INSTALLDIR ?= /usr/local/bin

SRC     = $(wildcard source/*.c)
OBJ     = $(SRC:.c=.o)
DEP     = $(SRC:.c=.d)

all: $(BINB) $(BINR)

-include $(DEP)

$(BINB): $(OBJ)
	@echo "$^ -> $@"
	$(ECHO)$(CC) -o $@ $^ $(LDFLAGS)
ifeq ($(DEBUG), 0)
	$(ECHO)$(STRIP) $@
endif

$(BINR): $(BINB)
	@echo "$^ -> $@"
	$(ECHO)ln -s $(BINB) $(BINR)

%.o: %.c
	@echo "$< -> $@"
	$(ECHO)$(CC) $(CFLAGS) -MMD -o $@ -c $<

.PHONY: clean install

install: $(BINB) $(BINR)
	@echo "$(BINB) -> $(INSTALLDIR)"
	$(ECHO)sudo cp $(BINB) $(INSTALLDIR)
	@echo "$(BINR) -> $(INSTALLDIR)"
	$(ECHO)sudo ln -sf $(INSTALLDIR)/$(BINB) $(INSTALLDIR)/$(BINR)

clean:
	@rm -f source/*.o source/*.d $(BINR) $(BINB)
