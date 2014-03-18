INCLUDE =-I./  -I../../include/time -I../../include/net\


CFLAGS = -fPIC -g

OUTPUT  = libtimerqueue.a

OBJS    = \
		timer_heap_queue.o  time_value.o
		  
	
all:$(OUTPUT) 

.SUFFIXES: .o .cpp
.cpp.o:
	$(CXX) $(CFLAGS) $(INCLUDE) -c $^ 

.o:
	$(CXX) $(CFLAGS) $(INCLUDE) -o $@ $^

$(OUTPUT): $(OBJS)
	ar r $@ $^

clean:
	rm -f *.o *.~ *.bak
	rm -f $(OUTPUT)

dep:
	mkdep $(INCLUDE) $(SOURCES) > .depend

install:
	cp -fv libtimerqueue.a ../../libs

ifneq ($(wildcard .depend),)
	include .depend
endif
