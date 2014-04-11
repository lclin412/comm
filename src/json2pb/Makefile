
Target:=libjson2pb.a

INC_PATH:=-I../../include/json2pb

CPP_FILES:=json2pb.cc

LIB_PATH:=

LIBS:= ../../libs/libjansson.a

OBJ_FILES:=$(CPP_FILES:.c=.o)
OBJ_FILES:=$(OBJ_FILES:.cc=.o)

main:$(Target)
	rm -rf *.o

$(Target):$(OBJ_FILES)
	ar r $(Target) $(OBJ_FILES) $(LIB_PATH) $(LIBS)

%.o:%.c
	gcc -o $@ -c $< $(INC_PATH)

%.o:%.cc
	$(CC) -o $@ -c $< $(INC_PATH)

install:
	mv $(Target) ../../libs/

clean:
	-rm $(Target) $(OBJ_FILES)
