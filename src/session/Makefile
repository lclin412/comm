
Target:=libsession_store.a

CC:=g++

CFLAGS:=-std=c++0x

INC_PATH:=-I../../include/session -I../../include/libmemcached

CPP_FILES:= session_store.cpp

OBJ_FILES:= $(CPP_FILES:.cpp=.o)

main:$(Target)
	rm -rf *.o

$(Target):$(OBJ_FILES)
	ar r $(Target) $(OBJ_FILES)

%.o:%.cpp
	$(CC) $(CFLAGS) -o $@ -c $< $(INC_PATH)

install:
	cp $(Target) ../../libs

clean:
	rm -rf $(Target) $(OBJ_FILES)
