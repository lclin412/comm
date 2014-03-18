INCLUDE =-I./  -I../../include/net/ \
		 -I../../include/time/ \
		 -I../../include/serialize \
                 -I../../include 


CFLAGS = -fPIC -Wno-deprecated -g
CFLAGS_IMSERVER = -fPIC -Wno-deprecated -g -D__IM_SERVER_CONFIG__


all:
	g++ $(CFLAGS) $(INCLUDE) -c net_svr_reactor.cpp
	g++ $(CFLAGS) $(INCLUDE) -c svr_reactor_facade.cpp
	g++ $(CFLAGS) $(INCLUDE) -c tcp_handle_base.cpp
	g++ $(CFLAGS) $(INCLUDE) -c epoll_svr_reactor.cpp
	g++ $(CFLAGS) $(INCLUDE) -c svr_reactor_facade_epoll.cpp
	ar r libnetbase.a net_svr_reactor.o svr_reactor_facade.o tcp_handle_base.o
	ar r libnetbase_epoll.a epoll_svr_reactor.o svr_reactor_facade_epoll.o tcp_handle_base.o
	rm -rf *.o
	g++ $(CFLAGS_IMSERVER) $(INCLUDE) -c epoll_svr_reactor.cpp
	g++ $(CFLAGS_IMSERVER) $(INCLUDE) -c svr_reactor_facade_epoll.cpp
	g++ $(CFLAGS_IMSERVER) $(INCLUDE) -c tcp_handle_base.cpp
	ar r libnetbase_epoll_imserver.a epoll_svr_reactor.o svr_reactor_facade_epoll.o tcp_handle_base.o
	rm -rf *.o


clean:
	rm -f *.o *.~ *.bak
	rm -f *.a

dep:
	mkdep $(INCLUDE) $(SOURCES) > .depend

install:
	cp -fv *.a ../../libs

ifneq ($(wildcard .depend),)
	include .depend
endif
