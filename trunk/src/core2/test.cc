#include <stdlib.h>
#include <iostream>
#include <sstream>
#include "rappture.h"
#include "Lookup.h"

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

    Rappture::Lookup2<int,int> num2id;
    num2id.get(4,NULL).value() = 1;
    num2id.get(105,NULL).value() = 2;
    num2id.get(69,NULL).value() = 3;
    num2id.get(95,NULL).value() = 4;

    Rappture::Outcome err = foo(1);
    if (err) {
        std::cout << err.remark() << std::endl;
        std::cout << err.context() << std::endl;
    } else {
        std::cout << "foo ok" << std::endl;
    }

    std::cout << num2id.stats();
    Rappture::LookupEntry2<int,int> entry = num2id.first();
    while (!entry.isNull()) {
        std::cout << " " << entry.key() << " = " << entry.value() << std::endl;
        entry.next();
    }

    Rappture::Lookup<double> str2dbl;
    const char *one = "testing";
    const char *two = "another";
    str2dbl[one] = 2.0;
    str2dbl[two] = 5.75;

    std::cout << str2dbl.stats();
    Rappture::LookupEntry<double> entry2 = str2dbl.first();
    while (!entry2.isNull()) {
        std::cout << " " << entry2.key() << " = " << entry2.value() << std::endl;
        entry2.next();
    }

    exit(0);
}
