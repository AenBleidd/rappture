OBJ_NANOVIS = nanovis.o Socket.o RenderVertexArray.o Plane.o ConvexPolygon.o Vector4.o Vector3.o Mat4x4.o \
		Texture1D.o Texture3D.o Volume.o Texture2D.o ParticleSystem.o Sphere.o Color.o \
		PerfQuery.o TransferFunction.o ControlPoint.o ColorGradient.o ColorPaletteWindow.o\
		ColorGradientGLUTWindow.o TransferFunctionGLUTWindow.o MainWindow.o Event.o
OBJ_CLIENT = Socket.o ClientSocket.o RenderClient.o

AUXSRC = config.h define.h global.h

SOCKETSRC = ./socket
SORTSRC = ./sort
TFSRC = ./transfer-function

LIB_NANOVIS = -ltcl8.4 -lGL -lglut -lglui -lGLEW -lCg -lCgGL -pthread -lstdc++
LIB_CLIENT = -lGL -lglut -pthread
CFLAG = -g -c -Wall -I../src2/core

all: nanovis client

clean:
	rm -f *.o nanovis 

nanovis: $(OBJ_NANOVIS)
	gcc -g -o nanovis $(OBJ_NANOVIS) $(LIB_NANOVIS) ../src2/core/*.o

ColorGradient.o: Color.o $(TFSRC)/ColorGradient.cpp
	gcc $(CFLAG) $(TFSRC)/ColorGradient.cpp

ColorPaletteWindow.o: Color.o ControlPoint.o $(TFSRC)/ColorPaletteWindow.cpp
	gcc $(CFLAG) $(TFSRC)/ColorPaletteWindow.cpp

ColorGradientGLUTWindow.o: ControlPoint.o ColorGradient.o $(TFSRC)/ColorGradientGLUTWindow.cpp
	gcc $(CFLAG) $(TFSRC)/ColorGradientGLUTWindow.cpp

TransferFunctionGLUTWindow.o: ColorGradientGLUTWindow.o $(TFSRC)/TransferFunctionGLUTWindow.cpp
	gcc $(CFLAG) $(TFSRC)/TransferFunctionGLUTWindow.cpp

MainWindow.o: TransferFunctionGLUTWindow.o ColorGradientGLUTWindow.o ColorPaletteWindow.o $(TFSRC)/MainWindow.cpp
	gcc $(CFLAG) $(TFSRC)/MainWindow.cpp
	
Socket.o: $(SOCKETSRC)/Socket.h $(SOCKETSRC)/Socket.cpp
	gcc $(CFLAG) $(SOCKETSRC)/Socket.cpp

ClientSocket.o: $(SOCKETSRC)/ClientSocket.h $(SOCKETSRC)/ClientSocket.cpp Socket.o
	gcc $(CFLAG) $(SOCKETSRC)/ClientSocket.cpp

RenderClient.o: $(SOCKETSRC)/RenderClient.h $(SOCKETSRC)/RenderClient.cpp ClientSocket.o
	gcc $(CFLAG) $(SOCKETSRC)/RenderClient.cpp

RenderVertexArray.o: RenderVertexArray.h RenderVertexArray.cpp
	gcc $(CFLAG) RenderVertexArray.cpp

client: $(OBJ_CLIENT) 
	gcc -g -o client $(OBJ_CLIENT) $(LIB_CLIENT)

Color.o: Color.cpp
	gcc $(CFLAG) Color.cpp

Event.o: Event.cpp
	gcc $(CFLAG) Event.cpp

ControlPoint.o: $(TFSRC)/ControlPoint.cpp
	gcc $(CFLAG) $(TFSRC)/ControlPoint.cpp

Sphere.o: Vector3.o Color.o
	gcc $(CFLAG) Sphere.cpp

TransferFunction.o: TransferFunction.cpp
	gcc $(CFLAG) TransferFunction.cpp

Texture1D.o: Texture1D.cpp
	gcc $(CFLAG) Texture1D.cpp

Texture2D.o: Texture2D.cpp $(AUXSRC)
	gcc $(CFLAG) Texture2D.cpp

Texture3D.o: Texture3D.cpp $(AUXSRC)
	gcc $(CFLAG) Texture3D.cpp

ParticleSystem.o: ParticleSystem.cpp $(AUXSRC)
	gcc $(CFLAG) ParticleSystem.cpp

Volume.o: Texture3D.o Volume.cpp $(AUXSRC)
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

PerfQuery.o: PerfQuery.cpp
	gcc $(CFLAG) PerfQuery.cpp

nanovis.o: nanovis.h nanovis.cpp ParticleSystem.o $(AUXSRC)
	gcc $(CFLAG) nanovis.cpp
