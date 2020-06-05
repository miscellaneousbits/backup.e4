BIN	= backrest.extfs
CC	= gcc
STRIP	= strip

#ECHO	=
#OFLAGS	= -g
ECHO	= @
OFLAGS	= -O3 -flto
CFLAGS	= $(OFLAGS) -Wall -fdata-sections -ffunction-sections -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
LDFLAGS	= $(OFLAGS) -Wl,--gc-sections -Wl,-Map,$(BIN).map
LDFLAGS	+= -lz

SRC	= $(wildcard *.c)
OBJ	= $(SRC:.c=.o)
DEP	= $(SRC:.c=.d)

all: $(BIN)

-include $(DEP)

$(BIN): $(OBJ)
	@echo "$^ -> $@"
	$(ECHO)$(CC) -o $@ $^ $(LDFLAGS)
	$(ECHO)$(STRIP) $@

%.o: %.c
	@echo "$< -> $@"
	$(ECHO)$(CC) $(CFLAGS) -MMD -o $@ -c $<

.PHONY: clean install

install: $(BIN)
	sudo cp $(BIN) /usr/local/bin

clean:
	@rm -f *.o *.d *.map $(BIN)
