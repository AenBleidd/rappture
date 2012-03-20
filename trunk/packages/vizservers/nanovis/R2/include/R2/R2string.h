/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef R2_STRING_H
#define R2_STRING_H

#include <string.h>

#include <R2/R2.h>

class R2string
{
public:
    R2string();

    R2string(const char *str);

    R2string(const R2string& string);

    ~R2string();

    operator char*();

    operator const char*() const;

    R2string& operator=(const R2string& string);

    R2string& operator=(const char* string);

    int getLength() const;

    friend R2string operator+(const R2string& string1, const R2string& string2);

    friend R2string operator+(const R2string& string1, const char* string2);

    friend bool operator==(const R2string& string1, const char* string2);

private:
    void set(const char *string, int length);

    char *_string;
    int _length;
};

inline R2string::operator char*()
{
    return _string;
}

inline R2string::operator const char* () const
{
    return _string;
}

inline int R2string::getLength() const
{
    return _length;
}

inline void R2string::set(const char* string, int length)
{
    if (_string) {
        delete [] _string;
        _string = NULL;
    }

    _length = length;
    if (_length) {
        _string = new char[length + 1];
        strcpy(_string, string);
    }
}

inline bool operator==(const R2string& string1, const char* string2)
{
    return (!strcmp(string1._string, string2));
}

#endif

