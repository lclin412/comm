INCLUDE =-I./  -I../../include/mysql/ \
		 -I/usr/include/mysql \
		 -I../../include/base/\


CFLAGS = -fPIC -g

OUTPUT  = libmysqlstorage.a

OBJS    = \
		  db_operator_mysql.o  mysqlresultset.o  mysqlstorage.o\

		  
	
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
	cp -fv libmysqlstorage.a ../../libs

ifneq ($(wildcard .depend),)
	include .depend
endif
