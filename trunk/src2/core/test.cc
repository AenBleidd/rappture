#include <stdlib.h>
#include <iostream>
#include <sstream>
#include "rappture.h"
#include "Dictionary.h"

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

    Rappture::Dictionary<int,int> str2id;
    str2id.get(4,NULL).value() = 1;
    str2id.get(105,NULL).value() = 2;
    str2id.get(69,NULL).value() = 3;
    str2id.get(95,NULL).value() = 4;

    std::cout << str2id.stats();

    Rappture::DictEntry<int,int> entry = str2id.first();
    while (!entry.isNull()) {
        std::cout << " " << entry.key() << " = =" << entry.value() << std::endl;
        entry.next();
    }

    exit(0);
}
