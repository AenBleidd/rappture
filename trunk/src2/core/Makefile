CFLAGS = -g
all: viz

viz: viz.o libviz.a
	g++ $(CFLAGS) -o viz viz.o libviz.a

libviz.a: RpOutcome.o RpMesh1D.o RpField1D.o RpMeshTri2D.o RpMeshRect3D.o RpFieldRect3D.o RpMeshPrism3D.o RpFieldPrism3D.o RpSerialBuffer.o RpSerializer.o RpSerializable.o
	ar crv $@ $^

viz.o: viz.cc
	g++ $(CFLAGS) -c -o viz.o viz.cc

test: test.o Ptr.o Outcome.o Lookup.o
	g++ $(CFLAGS) -o test test.o Ptr.o Outcome.o Lookup.o

docs:
	if test ! -d ../../docs/rappture2; then \
	  mkdir ../../docs; \
	  mkdir ../../docs/rappture2; \
	fi
	doxygen

test.o: test.cc Ptr.h
	g++ $(CFLAGS) -c -o test.o test.cc

Ptr.o: Ptr.h Ptr.cpp
	g++ $(CFLAGS) -c -o Ptr.o Ptr.cpp

RpOutcome.o: RpOutcome.cc RpPtr.h
	g++ $(CFLAGS) -c -o RpOutcome.o RpOutcome.cc

Lookup.o: Lookup.cpp Ptr.h
	g++ $(CFLAGS) -c -o Lookup.o Lookup.cpp

RpMesh1D.o: RpMesh1D.cc RpMesh1D.h RpNode.h
	g++ $(CFLAGS) -c -o RpMesh1D.o RpMesh1D.cc

RpField1D.o: RpField1D.cc RpField1D.h RpMesh1D.h RpNode.h
	g++ $(CFLAGS) -c -o RpField1D.o RpField1D.cc

RpMeshTri2D.o: RpMeshTri2D.cc RpMeshTri2D.h RpNode.h
	g++ $(CFLAGS) -c -o RpMeshTri2D.o RpMeshTri2D.cc

RpFieldTri2D.o: RpFieldTri2D.cc RpFieldTri2D.h RpNode.h
	g++ $(CFLAGS) -c -o RpFieldTri2D.o RpFieldTri2D.cc

RpMeshRect3D.o: RpMeshRect3D.cc RpMeshRect3D.h RpNode.h
	g++ $(CFLAGS) -c -o RpMeshRect3D.o RpMeshRect3D.cc

RpFieldRect3D.o: RpFieldRect3D.cc RpFieldRect3D.h RpNode.h
	g++ $(CFLAGS) -c -o RpFieldRect3D.o RpFieldRect3D.cc

RpMeshPrism3D.o: RpMeshPrism3D.cc RpMeshPrism3D.h RpNode.h
	g++ $(CFLAGS) -c -o RpMeshPrism3D.o RpMeshPrism3D.cc

RpFieldPrism3D.o: RpFieldPrism3D.cc RpFieldPrism3D.h RpNode.h
	g++ $(CFLAGS) -c -o RpFieldPrism3D.o RpFieldPrism3D.cc

RpSerialBuffer.o: RpSerialBuffer.cc RpSerialBuffer.h
	g++ $(CFLAGS) -c -o RpSerialBuffer.o RpSerialBuffer.cc

RpSerializer.o: RpSerializer.cc RpSerializer.h
	g++ $(CFLAGS) -c -o RpSerializer.o RpSerializer.cc

RpSerializable.o: RpSerializable.cc RpSerializable.h
	g++ $(CFLAGS) -c -o RpSerializable.o RpSerializable.cc

clean:
	rm -f viz test *.o
