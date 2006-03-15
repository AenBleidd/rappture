#include <stdlib.h>
#include <iostream>
#include <sstream>
#include "RpPtr.h"
#include "RpOutcome.h"
#include "RpNode.h"
#include "RpField1D.h"
#include "RpMeshRect3D.h"
#include "RpFieldRect3D.h"
#include "RpSerialBuffer.h"
////#include "RpSerializable.h"

class Foo {
  char *desc;

public:
  Foo(char *d) : desc(d) {
    std::cout << "created " << desc << std::endl;
  }
  ~Foo() {
    std::cout << "destroyed " << desc << std::endl;
  }
};

Rappture::Outcome
foo(int code) {
    Rappture::Outcome status;
    if (code) {
        status.error("Bad status code");
        status.addContext("while in foo");
    }
    return status;
}

void
showmesh(Rappture::Mesh1D& mesh) {
    for (int i=0; i < mesh.size(); i++) {
        std::cout << mesh.at(i).id() << "=" << mesh.at(i).x() << " ";
    }
    std::cout << std::endl;
}

int
main(int argc, char* argv[]) {
    Rappture::Ptr<Foo> ptr(new Foo("a"));
    Rappture::Ptr<Foo> ptr2(new Foo("barney"));

    std::cout << "copy ptr(barney) -> ptr(a)" << std::endl;
    ptr = ptr2;

    std::cout << "clear ptr(barney)" << std::endl;
    ptr2.clear();
    std::cout << "clear ptr(barney)" << std::endl;
    ptr.clear();

    Rappture::Field1D f;
    f.define(1.1, 1);
    f.define(3.3, 2);
    f.define(7.7, 3);

    std::cout << "@   0 = " << f.value(0) << std::endl;
    std::cout << "@ 2.2 = " << f.value(2.2) << std::endl;
    std::cout << "@ 5.5 = " << f.value(5.5) << std::endl;
    std::cout << "@ 5.6 = " << f.value(5.6) << std::endl;
    std::cout << "@ 7.6 = " << f.value(7.6) << std::endl;
    std::cout << "@ 7.7 = " << f.value(7.7) << std::endl;
    std::cout << "@ 8.8 = " << f.value(8.8) << std::endl;

    Rappture::SerialBuffer buffer;
    buffer.writeChar('a');
    buffer.writeInt(1051);
    buffer.writeDouble(2.51e12);
    buffer.writeString("xyz");
    buffer.writeString("pdq");

    buffer.rewind();
    std::cout << "buffer: ";
    std::cout << buffer.readChar() << " ";
    std::cout << buffer.readInt() << " ";
    std::cout << buffer.readDouble() << " ";
    std::cout << buffer.readString() << " ";
    std::cout << buffer.readString() << std::endl;

    Rappture::Mesh1D uni(1.0,5.0,16);
    showmesh(uni);

    Rappture::Mesh1D uniy(0.0,2.0,10);
    Rappture::Mesh1D uniz(-1.0,1.0,20);
    Rappture::Ptr<Rappture::MeshRect3D> m3dPtr(
        new Rappture::MeshRect3D(uni, uniy, uniz)
    );
    Rappture::FieldRect3D f3d(m3dPtr);

    int n=0;
    for (int iz=0; iz < f3d.size(Rappture::zaxis); iz++) {
        double zval = f3d.nodeAt(Rappture::zaxis, iz).x();
        for (int iy=0; iy < f3d.size(Rappture::yaxis); iy++) {
            double yval = f3d.nodeAt(Rappture::yaxis, iy).x();
            for (int ix=0; ix < f3d.size(Rappture::xaxis); ix++) {
                double xval = f3d.nodeAt(Rappture::xaxis, ix).x();
                f3d.define(n++, xval*yval*zval);
            }
        }
    }

    std::cout << "at (1,0,-0.5) " << f3d.value(1.0,0.0,-0.5) << std::endl;
    std::cout << "at (2,1,-0.5) " << f3d.value(2.0,1.0,-0.5) << std::endl;
    std::cout << "at (3,0.7,-0.2) " << f3d.value(3.0,0.7,-0.2) << std::endl;
    std::cout << "at (5,2,1) " << f3d.value(5.0,2.0,1.0) << std::endl;

    Rappture::Ptr<Foo> ptr3(new Foo("q"));
    std::ostringstream ss;
    ss << (void*)ptr3.pointer();
    std::cout << ss << std::endl;

    Rappture::Outcome status = foo(1);
    if (status) {
        status.addContext("while in main program");
        std::cout << status.remark() << std::endl;
        std::cout << status.context() << std::endl;
        exit(1);
    }
    exit(0);
}
