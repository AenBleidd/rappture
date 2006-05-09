OBJ_NANOVIS = nanovis.o Socket.o RenderVertexArray.o Plane.o ConvexPolygon.o Vector4.o Vector3.o Mat4x4.o \
		Texture1D.o Texture3D.o Volume.o Texture2D.o ParticleSystem.o Sphere.o Color.o \
		PerfQuery.o TransferFunction.o ControlPoint.o ColorGradient.o ColorPaletteWindow.o\
		ColorGradientGLUTWindow.o TransferFunctionGLUTWindow.o MainWindow.o Event.o \
		Lic.o Renderable.o Camera.o ScreenSnapper.o VolumeRenderer.o PlaneRenderer.o 
OBJ_VOLUMERENDERER = Volume.o ConvexPolygon.o TransferFunction.o Mat4x4.o  Camera.o
OBJ_PLANERENDERER = Texture2D.o TransferFunction.o
OBJ_CLIENT = Socket.o ClientSocket.o RenderClient.o Event.o

AUXSRC = config.h define.h global.h

SOCKETSRC = ./socket
SORTSRC = ./sort
TFSRC = ./transfer-function
NANOSCALESRC = ./nanoscale

LIB_NANOVIS = -ltcl8.3 -L/opt/render/lib -lGL -lglut -lglui -lGLEW -lCg -lCgGL -pthread -lstdc++
LIB_CLIENT = -L/opt/render/lib -lGL -lglut -pthread -lstdc++
INCLUDES = -I../../src2/core -I/opt/render/include -I/autohome/u76/qiaow/Cg/usr/include
CFLAG = -g -c $(INCLUDES)

all: nanovis

clean:
	rm -f *.o nanovis client simclient

nanovis: $(OBJ_NANOVIS)
	gcc -g -o nanovis $(OBJ_NANOVIS) $(LIB_NANOVIS) ../../src2/core/*.o

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

#simclient: Event.o $(NANOSCALESRC)/EventPlayer.cpp
#	gcc -g $(INCLUDES) $(NANOSCALESRC)/EventPlayer.cpp $(NANOSCALESRC)/clientlib.cpp -o simclient Event.o $(LIB_CLIENT)
#	g++ -g $(INCLUDES) $(NANOSCALESRC)/EventPlayer.cpp $(NANOSCALESRC)/clientlib.cpp -o simclient Event.o

Color.o: Color.cpp Color.h
	gcc $(CFLAG) Color.cpp

Event.o: Event.cpp Event.h
	gcc $(CFLAG) Event.cpp

ControlPoint.o: $(TFSRC)/ControlPoint.cpp
	gcc $(CFLAG) $(TFSRC)/ControlPoint.cpp

Sphere.o: Renderable.o Color.o Sphere.cpp Sphere.h
	gcc $(CFLAG) Sphere.cpp

TransferFunction.o: TransferFunction.cpp
	gcc $(CFLAG) TransferFunction.cpp

Texture1D.o: Texture1D.cpp Texture1D.h
	gcc $(CFLAG) Texture1D.cpp

Texture2D.o: Texture2D.cpp Texture2D.h $(AUXSRC)
	gcc $(CFLAG) Texture2D.cpp

Texture3D.o: Texture3D.cpp Texture3D.h $(AUXSRC)
	gcc $(CFLAG) Texture3D.cpp

ParticleSystem.o: Renderable.o ParticleSystem.cpp $(AUXSRC)
	gcc $(CFLAG) ParticleSystem.cpp

Renderable.o: Vector3.o Renderable.cpp Renderable.h
	gcc $(CFLAG) Renderable.cpp

Camera.o: Vector3.o Camera.cpp Camera.h
	gcc $(CFLAG) Camera.cpp

ScreenSnapper.o: define.h ScreenSnapper.cpp ScreenSnapper.h
	gcc $(CFLAG) ScreenSnapper.cpp

Lic.o: Renderable.o Lic.cpp Lic.h $(AUXSRC)
	gcc $(CFLAG) Lic.cpp

Volume.o: Texture3D.o Volume.cpp Volume.h $(AUXSRC)
	gcc $(CFLAG) Volume.cpp

VolumeRenderer.o: $(OBJ_VOLUMERENDERER)  VolumeRenderer.h VolumeRenderer.cpp
	gcc $(CFLAG) VolumeRenderer.cpp

PlaneRenderer.o: $(OBJ_PLANERENDERER)  PlaneRenderer.h PlaneRenderer.cpp
	gcc $(CFLAG) PlaneRenderer.cpp

Mat4x4.o: Mat4x4.cpp Mat4x4.h
	gcc $(CFLAG) Mat4x4.cpp

Vector4.o: Vector4.cpp Vector4.h
	gcc $(CFLAG) Vector4.cpp

Vector3.o: Vector3.cpp Vector3.h
	gcc $(CFLAG) Vector3.cpp

Plane.o: Plane.cpp Plane.h
	gcc $(CFLAG) Plane.cpp

ConvexPolygon.o: ConvexPolygon.cpp ConvexPolygon.h
	gcc $(CFLAG) ConvexPolygon.cpp

PerfQuery.o: PerfQuery.cpp PerfQuery.h
	gcc $(CFLAG) PerfQuery.cpp

nanovis.o: nanovis.h nanovis.cpp ParticleSystem.o $(AUXSRC)
	gcc $(CFLAG) nanovis.cpp
