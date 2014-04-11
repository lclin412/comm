INCLUDE =-I./  -I../../include/util/ \


CFLAGS = -fPIC -g

OUTPUT  = libutil_crypto.a

OBJS    = md5.o aes.o     
	
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
	cp -fv libutil_crypto.a ../../libs

ifneq ($(wildcard .depend),)
	include .depend
endif
