INCLUDE =-I./  -I../../include/shm/ \


CFLAGS = -fPIC -g

OUTPUT  = libshm.a

OBJS    = \
		myshm.o

		  
	
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
	cp -fv libshm.a ../../libs

ifneq ($(wildcard .depend),)
	include .depend
endif
