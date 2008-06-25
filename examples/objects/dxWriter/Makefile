all: dxWrite

dxWrite: dxWrite.cc
	g++ -g -Wall -o $@ $< -lrappture 

clean:
	rm -rf dxWrite myDXFile.dx
