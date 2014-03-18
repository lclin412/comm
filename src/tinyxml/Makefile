INCLUDE =-I./  -I../../include/tinyxml/ \


CFLAGS = -fPIC -g

OUTPUT  = libtinyxml.a

OBJS    = \
		tinyxml.o tinyxmlerror.o  tinyxmlparser.o\
		tinystr.o
		  
	
all:$(OUTPUT) 

.SUFFIXES: .o .cpp
.cpp.o:
	$(CXX) $(CFLAGS) $(INCLUDE) -c $^ -DTIXML_USE_STL 

.o:
	$(CXX) $(CFLAGS) $(INCLUDE) -o $@ $^ -DTIXML_USE_STL

$(OUTPUT): $(OBJS)
	ar r $@ $^

clean:
	rm -f *.o *.~ *.bak
	rm -f $(OUTPUT)

dep:
	mkdep $(INCLUDE) $(SOURCES) > .depend

install:
	cp -fv libtinyxml.a ../../libs

ifneq ($(wildcard .depend),)
	include .depend
endif
