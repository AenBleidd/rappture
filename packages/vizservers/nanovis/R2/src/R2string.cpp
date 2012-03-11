/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <string.h>

#include <R2/R2string.h>

R2string::R2string() : 
    _string(NULL), 
    _length(0)
{
}

R2string::R2string(const char* str) :
    _string(NULL), _length(0)
{
    if (str != NULL) {
        set(str, (int)strlen(str));
    }
}

R2string::R2string(const R2string& string) :
    _string(NULL)
{
    set(string._string, string._length);
}

R2string::~R2string()
{
    delete [] _string;
}

R2string& R2string::operator=(const R2string& string)
{
    set(string._string, string._length);

    return *this;
}

R2string& R2string::operator=(const char* string)
{
    set(string, strlen(string));

    return *this;
}

R2string operator+(const R2string& string1, const R2string& string2)
{
    R2string ret;
    ret._length = string1._length + string2._length;
    ret._string = new char[ret._length + 1];
    
    strcpy(ret._string, string1._string);
    strcpy(ret._string + string1._length, string2._string);
    
    return ret;
}


R2string operator+(const R2string& string1, const char* string2)
{
    R2string ret;
    ret._length = string1._length + strlen(string2);
    ret._string = new char[ret._length + 1];
    
    strcpy(ret._string, string1._string);
    strcpy(ret._string + string1._length, string2);
    
    return ret;
}
