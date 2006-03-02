OBJ_NANOVIS = nanovis.o Socket.o RenderVertexArray.o Plane.o ConvexPolygon.o Vector4.o Vector3.o Mat4x4.o \
		Texture1D.o Texture3D.o ColorMap.o Volume.o

SOCKETSRC = ./socket
SORTSRC = ./sort

LIB_NANOVIS = -ltcl8.4 -lGL -lglut -lGLEW -lCg -lCgGL -pthread
LIB_CLIENT = -lGL -lglut -pthread
CFLAG = -g -c -Wall

all: nanovis 

clean:
	rm -f *.o nanovis 

nanovis: $(OBJ_NANOVIS)
	gcc -g -o nanovis $(OBJ_NANOVIS) $(LIB_NANOVIS)

Socket.o: $(SOCKETSRC)/Socket.h $(SOCKETSRC)/Socket.cpp
	gcc $(CFLAG) $(SOCKETSRC)/Socket.cpp

RenderVertexArray.o: RenderVertexArray.h RenderVertexArray.cpp
	gcc $(CFLAG) RenderVertexArray.cpp

#qdclient: $(OBJ_CLIENT) 
#	gcc -g -o qdclient $(OBJ_CLIENT) $(LIB_CLIENT)

ColorMap.o: ColorMap.cpp
	gcc $(CFLAG) ColorMap.cpp

Texture1D.o: Texture1D.cpp
	gcc $(CFLAG) Texture1D.cpp

Texture3D.o: Texture3D.cpp config.h
	gcc $(CFLAG) Texture3D.cpp

Volume.o: Texture3D.o Volume.cpp define.h
	gcc $(CFLAG) Volume.cpp

Mat4x4.o: Mat4x4.cpp
	gcc $(CFLAG) Mat4x4.cpp

Vector4.o: Vector4.cpp
	gcc $(CFLAG) Vector4.cpp

Vector3.o: Vector3.cpp
	gcc $(CFLAG) Vector3.cpp

Plane.o: Plane.cpp
	gcc $(CFLAG) Plane.cpp

ConvexPolygon.o: ConvexPolygon.cpp
	gcc $(CFLAG) ConvexPolygon.cpp

nanovis.o: nanovis.h nanovis.cpp config.h define.h
	gcc $(CFLAG) nanovis.cpp

