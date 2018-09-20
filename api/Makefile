CC = gcc
BNT_SRC = bnt_spi.c bnt_util.c bnt_api.c

BNT_OBJ = $(BNT_SRC:.c=.o)
BNT_LIB = libbnt.a
BNT_DEMO_OBJ = $(BNT_SRC:.c=.demo.o)
BNT_DEMO_LIB = libbnt.demo.a

CFLAGS = -DLINUX -Wall -g -I.
CFLAGS += -DDEBUG
CFLAGS += -DFPGA

LFLAGS =
CFLAGS_SO += $(CFLAGS) -fPIC
LFLAGS_SO += -shared $(LFLAGS) 

INCLUDES = $(wildcard *.h)
TARGETS = $(BNT_LIB) $(BNT_DEMO_LIB)

all: $(TARGETS)

%.o: %.c ${INCLUDES}
	$(CC) -c $(CFLAGS) $< -o $@

%.demo.o: %.c ${INCLUDES}
	$(CC) -c $(CFLAGS) -DDEMO $< -o $@

$(BNT_LIB): $(BNT_OBJ)
	$(AR) rcs $(BNT_LIB) $(LFLAGS) $(BNT_OBJ)

$(BNT_DEMO_LIB): $(BNT_DEMO_OBJ)
	$(AR) rcs $(BNT_DEMO_LIB) $(LFLAGS) $(BNT_DEMO_OBJ)

install:
#	cp -f $(BNT_LIB) $(INSTALL_PATH)/lib/

clean:
	rm -rf *.o
	rm -rf $(BNT_LIB) $(BNT_DEMO_LIB)
