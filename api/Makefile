CC = gcc
BNT_SRC = bnt_spi.c bnt_util.c bnt_api.c

BNT_OBJ = $(BNT_SRC:.c=.o)
BNT_LIB = libbnt.a
CFLAGS = -DLINUX -Wall -g -I.
LFLAGS =
CFLAGS_SO += $(CFLAGS) -fPIC
LFLAGS_SO += -shared $(LFLAGS) 
CFLAGS += -DDEBUG
CFLAGS += -DFPGA

INCLUDES = $(wildcard *.h)
TARGET = $(BNT_LIB)

all: $(TARGET)

%.o: %.c ${INCLUDES}
	$(CC) -c $(CFLAGS) $< -o $@
$(BNT_LIB): $(BNT_OBJ)
	$(AR) rcs $(BNT_LIB) $(LFLAGS) $(BNT_OBJ)

install:
#	cp -f $(BNT_LIB) $(INSTALL_PATH)/lib/

clean:
	rm -rf *.o
	rm -rf $(BNT_LIB)
