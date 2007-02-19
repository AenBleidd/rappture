#include <iostream>
#include <fstream>
#include "RpBuffer.h"

#define BUFF 30

int
testInit()
{
    int passed = 0;
    int tests = 0;

    /* =========================================================== */
    tests++;
    Rappture::Buffer *buffer1 = NULL;
    buffer1 = new Rappture::Buffer();

    if ( (buffer1 != NULL) &&
         (buffer1->size() == 0) ) {
        passed++;
        delete buffer1;
    }
    else {
        delete buffer1;
        printf("Error testInit: test1");
        return (tests == passed);
    }
    /* =========================================================== */

    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer2;
    if (buffer2.size() == 0) {
        passed++;
    }
    else {
        printf("Error testInit: test2");
        return (tests == passed);
    }
    /* =========================================================== */

    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer3 = Rappture::Buffer("hi my name is derrick",22);
    if (buffer3.size() == 22) {
        passed++;
    }
    else {
        printf("Error testInit: test3");
        return (tests == passed);
    }
    /* =========================================================== */

    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer4 = Rappture::Buffer("hi my name is derrick",-1);
    if (buffer4.size() == 21) {
        passed++;
    }
    else {
        printf("Error testInit: test4");
        return (tests == passed);
    }
    /* =========================================================== */

    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer5 = Rappture::Buffer("hi my name is derrick");
    if (buffer5.size() == 21) {
        passed++;
    }
    else {
        printf("Error testInit: test5");
        return (tests == passed);
    }
    /* =========================================================== */

    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer6 = Rappture::Buffer("hi my name is derrick");
    Rappture::Buffer buffer7 = buffer6;
    if ( (buffer6.size() == 21) && (buffer7.size() == buffer6.size()) ) {
        passed++;
    }
    else {
        printf("Error testInit: test6");
        return (tests == passed);
    }
    /* =========================================================== */

    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer8 = Rappture::Buffer("hi my name is derrick");
    Rappture::Buffer buffer9 = Rappture::Buffer(buffer8);
    if ( (buffer8.size() == 21) &&
         (buffer9.size() == buffer8.size()) &&
         (buffer8.bytes() != buffer9.bytes()) ) {
        passed++;
    }
    else {
        printf("Error testInit: test7");
        return (tests == passed);
    }
    /* =========================================================== */

    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer10 = Rappture::Buffer("hi my name is derrickhi my name is derrickhi my name is derrickhi my name is derrickhi my name is derrickhi my name is derrickhi my name is derrickhi my name is derrickhi my name is derrickhi my name is derrick");
    Rappture::Buffer buffer11 = Rappture::Buffer(buffer10);
    if ( (buffer10.size() == 210) &&
         (buffer10.size() == buffer11.size()) &&
         (buffer10.bytes() != buffer11.bytes()) ) {
        passed++;
    }
    else {
        printf("Error testInit: test8");
        return (tests == passed);
    }
    /* =========================================================== */

    return (tests == passed);
}

int testAppend()
{
    int passed = 0;
    int tests = 0;

    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer1;
    buffer1.append("hi my name is derrick",21);
    if (buffer1.size() == 21) {
        passed++;
    }
    else {
        printf("Error testAppend: length1 = %d\n",buffer1.size());
        return (tests == passed);
    }
    /* =========================================================== */

    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer2 = Rappture::Buffer("hi my name is derrick",190);
    buffer2.append("adding more text to the buffer",30);
    if (buffer2.size() == 220) {
        passed++;
        // printDBufferDetails(dbPtr);
    }
    else {
        printf("Error testAppend: length2 = %d\n",buffer2.size());
        return (tests == passed);
    }
    /* =========================================================== */

    /* =========================================================== */

    return (tests == passed);
}

int testRead()
{
    int passed = 0;
    int tests = 0;
    char b[BUFF];
    int bytesRead = 0;


    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer1;
    buffer1.append("abcdefghijklmnopqrstuvwxyz",26);
    bytesRead = buffer1.read(b, BUFF);

    /*
    std::cout << "num bytes read = :" <<  bytesRead << ":" << std::endl;
    std::cout << "value read in = :";
    for (int i = 0; i < bytesRead; i++) {
        std::cout << b[i];
    }
    std::cout << ":" << std::endl;
    */

    if (bytesRead == 26) {
        passed++;
    }
    else {
        printf("Error testRead 1\n");
        // printDBufferDetails(dbPtr);
        return (tests == passed);
    }

    /* =========================================================== */

    return (tests == passed);
}

int testCompress()
{
    int passed = 0;
    int tests = 0;
    int bytesRead = 0;

    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer1;
    Rappture::Outcome status1;
    buffer1.append("abcdefghijklmnopqrstuvwxyz",26);
    status1 = buffer1.encode(true,false);
    bytesRead = buffer1.size();

    // std::cout << status1.remark() << std::endl << status1.context() << std::endl;

    if (bytesRead == 46) {
        passed++;
    }
    else {
        printf("Error testCompress 1\n");
        // printDBufferDetails(dbPtr);
        return (tests == passed);
    }
    /* =========================================================== */
    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer2;
    Rappture::Outcome status2;
    buffer2.append("abcdefghijklmnopqrstuvwxyz",26);
    status2 = buffer2.encode(false,true);
    bytesRead = buffer2.size();

    // std::cout << status2.remark() << std::endl << status2.context() << std::endl;

    if (bytesRead == 37) {
        passed++;
    }
    else {
        printf("Error testCompress 2\n");
        // printDBufferDetails(dbPtr);
        return (tests == passed);
    }
    /* =========================================================== */
    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer3;
    Rappture::Outcome status3;
    buffer3.append("abcdefghijklmnopqrstuvwxyz",26);
    status2 = buffer3.encode(true,true);
    bytesRead = buffer3.size();

    // std::cout << status3.remark() << std::endl << status3.context() << std::endl;

    if (bytesRead == 65) {
        passed++;
    }
    else {
        printf("Error testCompress 3\n");
        // printDBufferDetails(dbPtr);
        return (tests == passed);
    }
    /* =========================================================== */

    return (tests == passed);
}

int testDeCompress()
{
    int passed = 0;
    int tests = 0;
    int bytesRead = 0;

    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer1;
    Rappture::Outcome status1;
    buffer1.append("abcdefghijklmnopqrstuvwxyz",26);
    status1 = buffer1.encode(true,false);
    status1 = buffer1.decode(true,false);
    bytesRead = buffer1.size();

    // std::cout << status1.remark() << std::endl << status1.context() << std::endl;

    if (bytesRead == 26) {
        passed++;
    }
    else {
        printf("Error testDeCompress 1\n");
        // printDBufferDetails(dbPtr);
        return (tests == passed);
    }
    /* =========================================================== */
    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer2;
    Rappture::Outcome status2;
    buffer2.append("abcdefghijklmnopqrstuvwxyz",26);
    status2 = buffer2.encode(false,true);
    status2 = buffer2.decode(false,true);
    bytesRead = buffer2.size();

    // std::cout << status2.remark() << std::endl << status2.context() << std::endl;

    if (bytesRead == 26) {
        passed++;
    }
    else {
        printf("Error testDeCompress 2\n");
        // printDBufferDetails(dbPtr);
        return (tests == passed);
    }
    /* =========================================================== */
    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer3;
    Rappture::Outcome status3;
    buffer3.append("abcdefghijklmnopqrstuvwxyz",26);
    status3 = buffer3.encode(true,true);
    status3 = buffer3.decode(true,true);
    bytesRead = buffer3.size();

    // std::cout << status3.remark() << std::endl << status3.context() << std::endl;

    if (bytesRead == 26) {
        passed++;
    }
    else {
        printf("Error testDeCompress 3\n");
        // printDBufferDetails(dbPtr);
        return (tests == passed);
    }
    /* =========================================================== */

    return (tests == passed);
}

int testDump()
{
    int passed = 0;
    int tests = 0;


    /* =========================================================== */
    tests++;
    char* filePath = "buffer1.txt";
    Rappture::Buffer buffer1;
    buffer1.append("abcdefghijklmnopqrstuvwxyz",26);
    buffer1.dump(filePath);

    std::ifstream inFile;
    std::ifstream::pos_type size = 0;
    char* memblock = NULL;

    inFile.open(filePath, std::ios::in | std::ios::ate | std::ios::binary);
    if (!inFile.is_open()) {
        std::cout << "Error testDump1 1" << std::endl;
        std::cout << "error while opening file" << std::endl;
        return (tests == passed);
    }

    size = inFile.tellg();
    memblock = new char [size];
    if (memblock == NULL) {
        std::cout << "Error testDump1 1" << std::endl;
        std::cout << "error allocating memory" << std::endl;
        inFile.close();
        return (tests == passed);
    }

    inFile.seekg(0,std::ios::beg);
    inFile.read(memblock,size);
    inFile.close();

    const char* buffer1bytes = buffer1.bytes();
    for (int i = 0; i < size; i++) {
        if (buffer1bytes[i] != memblock[i]) {
            std::cout << "Error testDump1 1" << std::endl;
            std::cout << "buffers not equal" << std::endl;
            return (tests == passed);
        }
    }

    delete [] memblock;
    passed++;
    /* =========================================================== */

    return (tests == passed);
}

int testLoad()
{
    int passed = 0;
    int tests = 0;


    /* =========================================================== */
    tests++;
    char* filePath = "buffer1.txt";
    Rappture::Buffer buffer1;
    Rappture::Buffer buffer1out;
    buffer1.append("abcdefghijklmnopqrstuvwxyz",26);
    buffer1.dump(filePath);
    buffer1out.load(filePath);
    int size = buffer1out.size();
    const char* b1bytes = buffer1.bytes();
    const char* b1obytes = buffer1out.bytes();

    if (size == 0) {
        std::cout << "Error testLoad1" << std::endl;
        std::cout << "zero bytes read from file" << std::endl;
        return (tests == passed);
    }


    for (int i = 0; i < size; i++) {
        if (b1bytes[i] != b1obytes[i]) {
            printf("Error testLoad1\n");
            std::cout << "buffers not equal" << std::endl;
            return (tests == passed);
        }
    }

    passed++;
    /* =========================================================== */

    return (tests == passed);
}

int testClear()
{
    int passed = 0;
    int tests = 0;


    /* =========================================================== */
    tests++;
    Rappture::Buffer buffer1;
    buffer1.append("abcdefghijklmnopqrstuvwxyz",26);
    int size1before = buffer1.size();
    buffer1.clear();
    int size1after = buffer1.size();

    if (size1before != 26) {
        std::cout << "Error testClear1" << std::endl;
        std::cout << "incorrect buffer size" << std::endl;
        return (tests == passed);
    }
    if (size1after != 0) {
        std::cout << "Error testClear1" << std::endl;
        std::cout << "clear failed buffer size" << std::endl;
        return (tests == passed);
    }

    passed++;
    /* =========================================================== */

    return (tests == passed);
}

int main()
{
    int passed = 0;

    passed += testInit();
    passed += testAppend();
    passed += testRead();
    passed += testCompress();
    passed += testDeCompress();
    passed += testDump();
    passed += testLoad();
    passed += testClear();

    if (passed != 8) {
        printf("failed: %d\n", passed);
    }
    else {
        printf("sucessful\n");
    }

    return 0;
}
