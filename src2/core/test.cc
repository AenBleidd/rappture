#include <stdlib.h>
#include <iostream>
#include <sstream>
#include "RpPtr.h"
#include "RpOutcome.h"
#include "RpNode.h"
#include "RpField1D.h"
#include "RpMeshTri2D.h"
#include "RpFieldTri2D.h"
#include "RpMeshRect3D.h"
#include "RpFieldRect3D.h"
#include "RpMeshPrism3D.h"
#include "RpFieldPrism3D.h"
#include "RpSerialBuffer.h"
#include "RpSerializer.h"

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

    Rappture::Ptr<Rappture::Mesh1D> uni(
        new Rappture::Mesh1D(1.0,5.0,16)
    );
    showmesh(*uni);

    Rappture::Mesh1D uniy(0.0,2.0,10);
    Rappture::Mesh1D uniz(-1.0,1.0,20);
    Rappture::Ptr<Rappture::MeshRect3D> m3dPtr(
        new Rappture::MeshRect3D(*uni, uniy, uniz)
    );
    Rappture::Ptr<Rappture::FieldRect3D> f3d(
        new Rappture::FieldRect3D(*uni, uniy, uniz)
    );

    int n=0;
    for (int iz=0; iz < f3d->size(Rappture::zaxis); iz++) {
        double zval = f3d->atNode(Rappture::zaxis, iz).x();
        for (int iy=0; iy < f3d->size(Rappture::yaxis); iy++) {
            double yval = f3d->atNode(Rappture::yaxis, iy).x();
            for (int ix=0; ix < f3d->size(Rappture::xaxis); ix++) {
                double xval = f3d->atNode(Rappture::xaxis, ix).x();
                f3d->define(n++, xval*yval*zval);
            }
        }
    }

    std::cout << "at (1,0,-0.5) " << f3d->value(1.0,0.0,-0.5) << std::endl;
    std::cout << "at (2,1,-0.5) " << f3d->value(2.0,1.0,-0.5) << std::endl;
    std::cout << "at (3,0.7,-0.2) " << f3d->value(3.0,0.7,-0.2) << std::endl;
    std::cout << "at (5,2,1) " << f3d->value(5.0,2.0,1.0) << std::endl;


    Rappture::MeshTri2D m2d;
    m2d.addNode( Rappture::Node2D(0.0,0.0) );
    m2d.addNode( Rappture::Node2D(1.0,0.0) );
    m2d.addNode( Rappture::Node2D(0.0,1.0) );
    m2d.addNode( Rappture::Node2D(1.0,1.0) );
    m2d.addNode( Rappture::Node2D(2.0,0.0) );
    m2d.addNode( Rappture::Node2D(2.0,1.0) );
    m2d.addCell(0,1,2);
    m2d.addCell(1,2,3);
    m2d.addCell(1,3,4);
    m2d.addCell(3,4,5);

    Rappture::FieldTri2D f2d(m2d);
    f2d.define(0, 0.0);
    f2d.define(1, 1.0);
    f2d.define(2, 1.0);
    f2d.define(3, 4.0);
    f2d.define(4, 2.0);
    f2d.define(5, 2.0);
    std::cout << "value @(0.1,0.1): " << f2d.value(0.1,0.1) << std::endl;
    std::cout << "value @(0.99,0.99): " << f2d.value(0.99,0.99) << std::endl;
    std::cout << "value @(1.99,0.01): " << f2d.value(1.99,0.01) << std::endl;
    std::cout << "value @(2.01,0.02): " << f2d.value(2.01,1.02) << std::endl;

    Rappture::FieldPrism3D fp3d(m2d, uniy);
    for (int i=0; i < 10; i++) {
        fp3d.define(i*6+0, (i+1)*0.0);
        fp3d.define(i*6+1, (i+1)*1.0);
        fp3d.define(i*6+2, (i+1)*1.0);
        fp3d.define(i*6+3, (i+1)*4.0);
        fp3d.define(i*6+4, (i+1)*2.0);
        fp3d.define(i*6+5, (i+1)*2.0);
    }
    std::cout << "value @(0.1,0.1,0.01): " << fp3d.value(0.1,0.1,0.01) << std::endl;
    std::cout << "value @(0.1,0.1,0.21): " << fp3d.value(0.1,0.1,0.21) << std::endl;
    std::cout << "value @(0.1,0.1,0.41): " << fp3d.value(0.1,0.1,0.41) << std::endl;
    std::cout << "value @(0.99,0.99,0.01): " << fp3d.value(0.99,0.99,0.01) << std::endl;
    std::cout << "value @(0.99,0.99,0.21): " << fp3d.value(0.99,0.99,0.21) << std::endl;
    std::cout << "value @(0.99,0.99,0.41): " << fp3d.value(0.99,0.99,0.41) << std::endl;


    std::cout << "original mesh:" << std::endl;
    showmesh( *uni );
    Rappture::Serializer objs;
    objs.add( uni.pointer() );
    Rappture::Ptr<Rappture::SerialBuffer> buffer = objs.serialize();

    Rappture::Serializer objs2;
    Rappture::Outcome err = objs2.deserialize(buffer->bytes(), buffer->size());

    if (err) {
        err.addContext("while in main program");
        std::cout << err.remark() << std::endl;
        std::cout << err.context() << std::endl;
        exit(1);
    }

    Rappture::Ptr<Rappture::Serializable> uni2ptr = objs2.get(0);
    Rappture::Ptr<Rappture::Mesh1D> uni2( (Rappture::Mesh1D*)uni2ptr.pointer() );
    std::cout << "reconstituted mesh:" << std::endl;
    showmesh( *uni2 );

    exit(0);
}
