all: viz

viz: viz.o RpOutcome.o RpMesh1D.o RpField1D.o RpMeshRect3D.o RpFieldRect3D.o
	g++ -g -o viz viz.o RpOutcome.o RpMesh1D.o RpField1D.o RpMeshRect3D.o RpFieldRect3D.o

viz.o: viz.cc
	g++ -c -g -o viz.o viz.cc

test: test.o RpOutcome.o RpMesh1D.o RpField1D.o RpMeshRect3D.o RpFieldRect3D.o RpSerialBuffer.o
	g++ -g -o test test.o RpOutcome.o RpMesh1D.o RpField1D.o RpMeshRect3D.o RpFieldRect3D.o RpSerialBuffer.o

test.o: test.cc RpPtr.h
	g++ -c -g -o test.o test.cc

RpOutcome.o: RpOutcome.cc RpPtr.h
	g++ -c -g -o RpOutcome.o RpOutcome.cc

RpMesh1D.o: RpMesh1D.cc RpMesh1D.h RpNode.h
	g++ -c -g -o RpMesh1D.o RpMesh1D.cc

RpField1D.o: RpField1D.cc RpField1D.h RpMesh1D.h RpNode.h
	g++ -c -g -o RpField1D.o RpField1D.cc

RpMeshRect3D.o: RpMeshRect3D.cc RpMeshRect3D.h RpNode.h
	g++ -c -g -o RpMeshRect3D.o RpMeshRect3D.cc

RpFieldRect3D.o: RpFieldRect3D.cc RpFieldRect3D.h RpNode.h
	g++ -c -g -o RpFieldRect3D.o RpFieldRect3D.cc

RpSerialBuffer.o: RpSerialBuffer.cc RpSerialBuffer.h
	g++ -c -g -o RpSerialBuffer.o RpSerialBuffer.cc

RpSerializable.o: RpSerializable.cc RpSerializable.h
	g++ -c -g -o RpSerializable.o RpSerializable.cc

clean:
	rm -f viz test *.o
