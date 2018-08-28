INCLUDES = $(wildcard ../module/*.h)
INSTALL_PATH=/bnt

CC = gcc -g

TARGET =  bnt_access
TARGET += spidev_test
TARGET += bnt_gpioint
TARGET += bnt_getnonce
TARGET += bnt_reset
TARGET += bnt_devscan

ifeq ($(BNT_LIBDIR),)
    BNT_LIBDIR = ../api
endif

BNT_LIB = $(BNT_LIBDIR)/libbnt.a
WD_BASEDIR := $(wildcard $(WD_BASEDIR))

ifeq ($(BNT_LIBDIR),)
    BNT_LIBDIR = ../api
endif

BNT_LIB = $(BNT_LIBDIR)/libbnt.a
BNT_INC = -I../api 

CFLAGS += -DLINUX -Wall
LFLAGS += -lcrypto

all: $(TARGET)

bnt_access: bnt_access.c $(BNT_LIB)
	$(CC) -Wall $< $(BNT_LIB) $(LFLAGS) $(BNT_INC) -o $@

bnt_gpioint: bnt_gpioint.c $(BNT_LIB)
	$(CC) -Wall $< $(BNT_LIB) $(LFLAGS) $(BNT_INC) -o $@

bnt_getnonce: bnt_getnonce.c $(BNT_LIB)
	$(CC) -Wall $< $(BNT_LIB) $(LFLAGS) $(BNT_INC) -o $@

bnt_reset: bnt_reset.c $(BNT_LIB)
	$(CC) -Wall $< $(BNT_LIB) $(LFLAGS) $(BNT_INC) -o $@

bnt_devscan: bnt_devscan.c $(BNT_LIB)
	$(CC) -Wall $< $(BNT_LIB) $(LFLAGS) $(BNT_INC) -o $@

spitest: spidev_test.c 
	$(CC) -Wall $< $(LFLAGS) -o $@

install:
#	cp -f $(TARGET) $(INSTALL_PATH)/diag

clean:
	rm -rf $(TARGET)

