CC		= g++
CXXFLAGS	= -Wall -pipe -std=c++98 -fno-rtti -fno-exceptions -Wno-long-long -Wno-deprecated -g -DQCC_OS_LINUX -DQCC_OS_GROUP_POSIX -DQCC_CPU_X86
LIBS		= -lalljoyn -lstdc++ -lcrypto -lpthread -lrt -luci -lubus -lubox -lblobmsg_json -ljson-c
OBJS		= something.o SomethingListener.o Wifihelper.o LampHelper.o somethingd.o
SRCS		= something.cc SomethingListener.cc Wifihelper.cc LampHelper.cc somethingd.c


all: something

something: ${OBJS}
	${CC} ${LDFLAGS} ${LIBSRCS} -o something ${OBJS} ${LIBS}

clean:
	rm -f something *.o

