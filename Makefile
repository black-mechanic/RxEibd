CXX = arm-linux-gnueabihf-g++
CC =  arm-linux-gnueabihf-gcc
LD =  arm-linux-gnueabihf-ld
CXXFLAGS += -I/usr/arm-linux-gnueabihf/include -I/usr/arm-linux-gnueabihf/lib
LDFLAGS += -L. -L/usr/arm-linux-gnueabihf/lib -L/usr/arm-linux-gnueabihf/include

all: RxEibd
RxEibd: RxEibd.o common.o
	$(CXX) $(LDFLAGS) -o RxEibd RxEibd.o common.o -leibclient

RxEibd.o: RxEibd.cpp common.h
	$(CXX) $(CXXFLAGS) -c RxEibd.cpp 

common.o: common.c common.h eibclient.h
	$(CXX) $(CXXFLAGS) -c common.c
	
clean:
	rm -rf *.o

