CC = gcc -g -Wall -DGCC
INC = ../../api
#-Wno-deprecated
CC_FLAGS = -c -I. -I$(INC) -pipe -D_GNU_SOURCE -DLINUX -Wall
CC_FLAGS += -DFPGA
CC_FLAGS += -DDEMO

LD = gcc
LD_FLAGS = -I. -lm -ldl
LD_FLAGS += -lcrypto

OBJS = ConsFunc.o 
OBJS += bnt_getnonce.o

LIBS = ../../api/libbnt.a

TARGET = bnt_demo

all: $(TARGET)

%.o : %.c
	@echo Compiling : $<
	$(CC) $(CC_FLAGS) -o $@ $<
	@echo

$(TARGET): $(OBJS)
	$(LD) $(LD_FLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS) $(TARGET)

