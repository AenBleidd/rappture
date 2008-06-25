#include <iostream>
#include "RpSimpleBuffer.h"

int main()
{
    Rappture::SimpleBuffer<int> fbuf;
    float val = 0;

    fbuf.set(10);

    for (int i=0; i < 100; i++) {
        std::cout << "storing " << val << std::endl;
        fbuf.append(&val,1);
        // fbuf.show();
        val += 1.1;
    }

    fbuf.rewind();
    val = -1;

    while (!fbuf.eof()) {
        fbuf.read(&val,1);
        std::cout << "retrieving " << val << std::endl;
    }

    fbuf.seek(0,SEEK_SET);
    std::cout << "SEEK_SET 0 pos = " << fbuf.tell() << std::endl;;

    fbuf.seek(40,SEEK_CUR);
    std::cout << "SEEK_CUR 40 pos = " << fbuf.tell() << std::endl;;

    fbuf.seek(-20,SEEK_END);
    std::cout << "SEEK_END -20 pos = " << fbuf.tell() << std::endl;;

    Rappture::SimpleBuffer<int> cbuf;
    cbuf.move(fbuf);
    cbuf.show();

    fbuf.clear();
    return 0;
}
