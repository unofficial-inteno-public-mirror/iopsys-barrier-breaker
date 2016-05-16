CC		= g++
CXXFLAGS	= -Wall -pipe -std=c++98 -fno-rtti -fno-exceptions -Wno-long-long -Wno-deprecated -g -DQCC_OS_LINUX -DQCC_OS_GROUP_POSIX -DQCC_CPU_X86
LIBS		= -lpthread -lstdc++
OBJS		= iotx.o #tools.o ubus.o jsp.o
SRCS		= iotx.cc # tools.cc ubus.c jsp.c
LIBSRCS		= 
ISRCS		= 

all: iotx

iotx: ${OBJS}
	${CC} ${LDFLAGS} ${LIBSRCS} -o iotx ${OBJS} ${LIBS}

clean:
	rm -f iotx *.o

