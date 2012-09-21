/*
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef RAPPTURE_ACCESSOR_H
#define RAPPTURE_ACCESSOR_H

#include <cstring>

namespace Rappture {

template <class T>
class Accessor
{
public:
    Accessor ();
    Accessor (const T &o);
    ~Accessor();

    // get routine
    T operator() (void) const;

    // set routine
    void operator() (T val);

private:

    T _val;
};

template <class T>
Accessor<T>::Accessor()
{
}

template <class T>
T
Accessor<T>::operator() (void) const
{
    return _val;
}

template <class T>
void
Accessor<T>::operator() (T val)
{
    _val = val;
}

template <class T>
Accessor<T>::~Accessor()
{
}

// handle const char *'s gracefully?

template <> inline
Accessor<const char *>::Accessor()
{
    _val = NULL;
}

template <> inline
void
Accessor<const char *>::operator() (const char *val)
{
    if (val == NULL) {
        return;
    }

    size_t len = strlen(val);
    char *tmp = new char[len+1];
    if (tmp == NULL) {
        // raise error and exit
    }
    strncpy(tmp,val,len+1);

    if (_val) {
        delete[] _val;
    }

    _val = tmp;
}

template <> inline
Accessor<const char *>::~Accessor()
{
    if (_val) {
        delete[] _val;
    }
}

} // namespace Rappture

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif // RAPPTURE_ACCESSOR_H
