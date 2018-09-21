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
OBJS += bnt_demoutil.o

GETNONCE_OBJ = $(OBJS)
GETNONCE_OBJ += bnt_getnonce.o

SETNOTI_OBJ = $(OBJS)
SETNOTI_OBJ += bnt_setnoti.o

LIBS = ../../api/libbnt.demo.a

TARGET = bnt_demo
TARGET += bnt_setnoti

all: $(TARGET) 

%.o : %.c
	@echo Compiling : $<
	$(CC) $(CC_FLAGS) -o $@ $<
	@echo

bnt_demo: bnt_getnonce.c $(LIBS) $(GETNONCE_OBJ)
	$(LD) $(LD_FLAGS) -o $@ $(GETNONCE_OBJ) $(LIBS)

bnt_setnoti: bnt_setnoti.c $(LIBS) $(SETNOTI_OBJ)
	$(LD) $(LD_FLAGS) -o $@ $(SETNOTI_OBJ) $(LIBS)

#$(TARGET): $(OBJS) $(LIBS)
#	$(LD) $(LD_FLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS) $(GETNONCE_OBJ) $(SETNOTI_OBJ) $($(TARGET)

