CC		= g++
CXXFLAGS	= -Wall -pipe -std=c++98 -fno-rtti -fno-exceptions -Wno-long-long -Wno-deprecated -g -DQCC_OS_LINUX -DQCC_OS_GROUP_POSIX -DQCC_CPU_X86
LIBS		= -lalljoyn -lstdc++ -lcrypto -lpthread -lrt -luci -lubus -lubox -lblobmsg_json -ljson-c
OBJS		= wifimngr.o wireless.o tools.o ubus.o
SRCS		= wifimngr.cc wireless.cc tools.cc ubus.c
LIBSRCS		= 
ISRCS		= wifi.h common.h

all: wifimngr

wifimngr: ${OBJS}
	${CC} ${LDFLAGS} ${LIBSRCS} -o wifimngr ${OBJS} ${LIBS}

clean:
	rm -f wifimngr *.o

