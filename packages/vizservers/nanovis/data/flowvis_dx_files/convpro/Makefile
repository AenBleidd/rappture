SRC = main.cpp
OBJ = $(SRC:.cpp=.o) 

TARGET = mat2dx
CC=gcc
COMPILER_OPTIONS =-Wall
# CCFLAGS=-Wall -O3
INCLUDES = \
	-I.

LIBS = -lm \
	-lstdc++ 
	

# default: def $(TARGET)
default: $(TARGET)
	
.cpp.o:
	$(CC) $(INCLUDES) $(LIBS)  $(COMPILER_OPTIONS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) -g -o $(TARGET) $(OBJ) $(LIBS) 

depend: dep

dep:
	makedepend -- $(CFLAGS) -- $(INCLUDES) $(SRC)

clean:
	rm -f *.o $(TARGET) Makefile.bak



