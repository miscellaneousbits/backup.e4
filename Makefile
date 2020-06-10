BINB    = backup.e4
BINR    = restore.e4
CC      = gcc
STRIP   = strip

#ECHO   =
#OFLAGS = -g
ECHO    = @
OFLAGS  = -O3 -flto
CFLAGS  = $(OFLAGS) -Wall -fdata-sections -ffunction-sections -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
LDFLAGS = $(OFLAGS) -Wl,--gc-sections -static -lz

INSTALLDIR = /usr/local/bin

SRC     = $(wildcard *.c)
OBJ     = $(SRC:.c=.o)
DEP     = $(SRC:.c=.d)

all: $(BINB) $(BINR)

-include $(DEP)

$(BINB): $(OBJ)
	@echo "$^ -> $@"
	$(ECHO)$(CC) -o $@ $^ $(LDFLAGS)
	$(ECHO)$(STRIP) $@

$(BINR): $(BINB)
	@echo "$^ -> $@"
	$(ECHO)ln -s $(BINB) $(BINR)

%.o: %.c
	@echo "$< -> $@"
	$(ECHO)$(CC) $(CFLAGS) -MMD -o $@ -c $<

.PHONY: clean install

install: $(BINB) $(BINR)
	@echo "$(BINB) -> /usr/local/bin"
	$(ECHO)sudo cp $(BINB) /usr/local/bin
	@echo "$(BINR) -> $(INSTALLDIR)"
	$(ECHO)sudo ln -sf $(INSTALLDIR)/$(BINB) $(INSTALLDIR)/$(BINR)

clean:
	@rm -f *.o *.d *.map $(BINR) $(BINB)
