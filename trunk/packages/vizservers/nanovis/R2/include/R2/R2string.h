/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __R2_STRING_H__
#define __R2_STRING_H__

#include <R2/R2.h>

class R2string {
	R2char*		_string;
	R2int32		_length;

private :
	void set(const R2char* string, R2int32 length);
public :
	R2string();
	R2string(const char* str);
	R2string(const R2string& string);
    ~R2string();

	operator char*();
	operator const char* () const;
	R2string& operator=(const R2string& string);
	R2string& operator=(const char* string);
	R2int32 getLength() const;

	friend R2string operator+(const R2string& string1, const R2string& string2);
	friend R2string operator+(const R2string& string1, const char* string2);
	friend R2bool operator==(const R2string& string1, const char* string2);
};

inline R2string::operator char*()
{
	return _string;
}

inline R2string::operator const char* () const
{
	return _string;
}

inline R2int32 R2string::getLength() const
{
	return _length;
}

inline void R2string::set(const R2char* string, R2int32 length)
{
	if (_string)  {
		delete [] _string;
		_string = NULL;
	}

	_length = length;
	if (_length)
	{
		_string = new R2char[length + 1];
		strcpy(_string, string);
	}

}

inline R2bool operator==(const R2string& string1, const char* string2)
{
	return (!strcmp(string1._string, string2));
}

#endif // __R2_STRING_H__

