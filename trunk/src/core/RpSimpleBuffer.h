/*
 * ======================================================================
 *  Rappture::SimpleBuffer
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2008  Purdue Research Foundation
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 *
 *  This code is based on the Tcl_DString facility included in the
 *  Tcl source release, which includes the following copyright:
 *
 *  Copyright (c) 1987-1993 The Regents of the University of California.
 *  Copyright (c) 1994-1998 Sun Microsystems, Inc.
 *  Copyright (c) 2001 by Kevin B. Kenny.  All rights reserved.
 *
 *  This software is copyrighted by the Regents of the University of
 *  California, Sun Microsystems, Inc., Scriptics Corporation,
 *  and other parties.  The following terms apply to all files associated
 *  with the software unless explicitly disclaimed in individual files.
 *
 *  The authors hereby grant permission to use, copy, modify, distribute,
 *  and license this software and its documentation for any purpose, provided
 *  that existing copyright notices are retained in all copies and that this
 *  notice is included verbatim in any distributions. No written agreement,
 *  license, or royalty fee is required for any of the authorized uses.
 *  Modifications to this software may be copyrighted by their authors
 *  and need not follow the licensing terms described here, provided that
 *  the new terms are clearly indicated on the first page of each file where
 *  they apply.
 *
 *  IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
 *  FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 *  ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
 *  DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
 *  IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
 *  NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 *  MODIFICATIONS.
 *
 *  GOVERNMENT USE: If you are acquiring this software on behalf of the
 *  U.S. government, the Government shall have only "Restricted Rights"
 *  in the software and related documentation as defined in the Federal·
 *  Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 *  are acquiring the software on behalf of the Department of Defense, the
 *  software shall be classified as "Commercial Computer Software" and the
 *  Government shall have only "Restricted Rights" as defined in Clause
 *  252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
 *  authors grant the U.S. Government and others acting in its behalf
 *  permission to use and distribute the software in accordance with the
 *  terms specified in this license.·
 *
 * ======================================================================
 */

#ifndef RAPPTURE_SIMPLEBUFFER_H
#define RAPPTURE_SIMPLEBUFFER_H

#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cstdarg>

namespace Rappture {

template <class T>
class SimpleBuffer {
public:
    SimpleBuffer();
    SimpleBuffer(size_t nmemb);
    SimpleBuffer(const T* bytes, int nmemb=-1);
    SimpleBuffer(const SimpleBuffer& b);
    SimpleBuffer<T>& operator=(const SimpleBuffer<T>& b);
    SimpleBuffer     operator+(const SimpleBuffer& b) const;
    SimpleBuffer<T>& operator+=(const SimpleBuffer<T>& b);
    virtual ~SimpleBuffer();

    const T* bytes() const;
    size_t size() const;
    size_t nmemb() const;

    SimpleBuffer<T>& clear();
    int append(const T* bytes, int nmemb=-1);
    int appendf(const char *format, ...);
    int remove(int nmemb);
    size_t read(const T* bytes, size_t nmemb);
    int seek(long offset, int whence);
    int tell() const;
    size_t set(size_t nmemb);
    SimpleBuffer<T>& rewind();
    SimpleBuffer<T>& show();

    bool good() const;
    bool bad() const;
    bool eof() const;

    SimpleBuffer<T>& move(SimpleBuffer<T>& b);

protected:

    void bufferInit();
    void bufferFree();

private:

    /// Pointer to the memory that holds our buffer's data
    T* _buf;

    /// Position offset within the buffer's memory
    size_t _pos;

    /// Number of members stored in the buffer
    size_t _nMembStored;

    /// Total number of members available in the buffer
    size_t _nMembAvl;

    /// State of the last file like operation.
    bool _fileState;

    /// Minimum number of members is set to the number you can fit in 256 bytes
    const static int _minMembCnt=(256/sizeof(T));

    size_t __guesslen(const T* bytes);

};

typedef SimpleBuffer<char>   SimpleCharBuffer;
typedef SimpleBuffer<float>  SimpleFloatBuffer;
typedef SimpleBuffer<double> SimpleDoubleBuffer;

/**
 * Construct an empty SimpleBuffer.
 */
template<class T>
SimpleBuffer<T>::SimpleBuffer()
{
    bufferInit();
}


/**
 * Construct an empty SimpleBuffer of specified size.
 */
template<class T>
SimpleBuffer<T>::SimpleBuffer(size_t nmemb)
{
    bufferInit();

    if (nmemb == 0) {
        // ignore requests for sizes equal to zero
        return;
    }

    // buffer sizes less than min_size are set to min_size
    if (nmemb < (size_t) _minMembCnt) {
        nmemb = _minMembCnt;
    }

    if (set(nmemb) != nmemb) {
        return;
    }
    _nMembStored = nmemb;
}


/**
 * Construct a SimpleBuffer loaded with initial data.
 *
 * @param bytes pointer to bytes being stored.
 * @param nbytes number of bytes being stored.
 */
template<class T>
SimpleBuffer<T>::SimpleBuffer(const T* bytes, int nmemb)
{
    bufferInit();
    append(bytes,nmemb);
}


/**
 * Copy constructor
 * @param SimpleBuffer object to copy
 */
template<class T>
SimpleBuffer<T>::SimpleBuffer(const SimpleBuffer<T>& b)
{
    bufferInit();
    append(b.bytes(),b.nmemb());
}


/**
 * Assignment operator
 * @param SimpleBuffer object to copy
 */
template<class T>
SimpleBuffer<T>&
SimpleBuffer<T>::operator=(const SimpleBuffer<T>& b)
{
    bufferFree();
    bufferInit();
    append(b.bytes(),b.nmemb());
    return *this;
}


/**
 * Operator +
 * @param SimpleBuffer object to add
 */
template<class T>
SimpleBuffer<T>
SimpleBuffer<T>::operator+(const SimpleBuffer<T>& b) const
{
    SimpleBuffer<T> newBuffer(*this);
    newBuffer.operator+=(b);
    return newBuffer;
}


/**
 * Operator +=
 * @param SimpleBuffer object to add
 */
template<class T>
SimpleBuffer<T>&
SimpleBuffer<T>::operator+=(const SimpleBuffer<T>& b)
{
    append(b.bytes(),b.nmemb());
    return *this;
}


/**
 * Destructor
 */
template<class T>
SimpleBuffer<T>::~SimpleBuffer()
{
    bufferFree();
}


/**
 * Get the bytes currently stored in the buffer.  These bytes can
 * be stored, and used later to construct another Buffer to
 * decode the information.
 *
 * @return Pointer to the bytes in the buffer.
 */
template<class T>
const T*
SimpleBuffer<T>::bytes() const
{
    return _buf;
}


/**
 * Get the number of bytes currently stored in the buffer.
 * @return Number of the bytes used in the buffer.
 */
template<class T>
size_t
SimpleBuffer<T>::size() const
{
    return _nMembStored*sizeof(T);
}


/**
 * Get the number of members currently stored in the buffer.
 * @return Number of the members used in the buffer.
 */
template<class T>
size_t
SimpleBuffer<T>::nmemb() const
{
    return _nMembStored;
}


/**
 * Clear the buffer, making it empty.
 * @return Reference to this buffer.
 */
template<class T>
SimpleBuffer<T>&
SimpleBuffer<T>::clear()
{
    bufferFree();
    bufferInit();

    return *this;
}

/**
 * guess the length of a null terminated character buffer
 * @param pointer to the buffer to guess the length of
 * @return a guess of the length of the buffer.
 */
template<> inline
size_t
SimpleBuffer<char>::__guesslen(const char* bytes)
{
    return strlen(bytes);
}

/**
 * guess the length of a non-null terminated character buffer
 * @param pointer to the buffer to guess the length of
 * @return a guess of the length of the buffer.
 */
template<class T>
size_t
SimpleBuffer<T>::__guesslen(const T* bytes)
{
    return (sizeof(T));
}

/**
 * Append bytes to the end of this buffer
 * @param pointer to bytes to be added
 * @param number of bytes to be added
 * @return number of bytes appended.
 */
template<class T>
int
SimpleBuffer<T>::append(const T* bytes, int nmemb)
{
    size_t newMembCnt = 0;
    size_t nbytes = 0;

    void* dest = NULL;
    void const* src  = NULL;
    size_t size = 0;

    // User specified NULL buffer to append
    if ( (bytes == NULL) && (nmemb < 1) ) {
        return 0;
    }

    // FIXME: i think this needs to be division,
    // need to create test case with array of ints
    // i'm not sure we can even guess the number
    // bytes in *bytes.

    if (nmemb == -1) {
        // user signaled null terminated string
        // or that we should make an educated guess
        // at the length of the object.
        nbytes = __guesslen(bytes);
        nmemb = nbytes/sizeof(T);
    }

    if (nmemb <= 0) {
        // no data written, invalid option
        return nmemb;
    }

    newMembCnt = (size_t)(_nMembStored + nmemb);

    if (newMembCnt > _nMembAvl) {

        // buffer sizes less than min_size are set to min_size
        if (newMembCnt < (size_t) _minMembCnt) {
            newMembCnt = (size_t) _minMembCnt;
        }

        /*
         * Allocate a larger buffer for the string if the current one isn't
         * large enough. Allocate extra space in the new buffer so that there
         * will be room to grow before we have to allocate again.
         */
        size_t membAvl;
        membAvl = (_nMembAvl > 0) ? _nMembAvl : _minMembCnt;
        while (newMembCnt > membAvl) {
            membAvl += membAvl;
        }

        /*
         * reallocate to a larger buffer
         */
        if (set(membAvl) != membAvl) {
            return 0;
        }
    }

    dest = (void*) (_buf + _nMembStored);
    src  = (void const*) bytes;
    size = (size_t) (nmemb*sizeof(T));
    memcpy(dest,src,size);

    _nMembStored += nmemb;

    return nmemb;
}

/**
 * Append formatted bytes to the end of this buffer
 * @param pointer to bytes to be added
 * @param number of bytes to be added
 * @return number of bytes appended.
 */
template<class T>
int
SimpleBuffer<T>::appendf(const char *format, ...)
{
    size_t newMembCnt = 0;
    size_t nbytes = 0;
    int nmemb = 0;

    char* dest = NULL;
    size_t size = 0;
    size_t bytesAdded = 0;
    va_list arg;

    // User specified NULL format
    if (format == NULL) {
        return 0;
    }

    // FIXME: i think this needs to be division,
    // need to create test case with array of ints
    // i'm not sure we can even guess the number
    // bytes in *bytes.


    // add one for terminating null character
    nbytes = strlen(format) + 1;

    if (nbytes <= 0) {
        // no data written, invalid option
        return nbytes;
    }

    // FIXME: we need ceil of nbytes/sizeof(T), instead we add 1 for safety

    nmemb = nbytes/sizeof(T);
    if (nmemb == 0) {
        nmemb++;
    }

    newMembCnt = (size_t)(_nMembStored + nmemb);

    if (newMembCnt > _nMembAvl) {

        // buffer sizes less than min_size are set to min_size
        if (newMembCnt < (size_t) _minMembCnt) {
            newMembCnt = (size_t) _minMembCnt;
        }

        /*
         * Allocate a larger buffer for the string if the current one isn't
         * large enough. Allocate extra space in the new buffer so that there
         * will be room to grow before we have to allocate again.
         */
        size_t membAvl;
        membAvl = (_nMembAvl > 0) ? _nMembAvl : _minMembCnt;
        while (newMembCnt > membAvl) {
            membAvl += membAvl;
        }

        /*
         * reallocate to a larger buffer
         */
        if (set(membAvl) != membAvl) {
            return 0;
        }
    }

    dest = (char*) (_buf + _nMembStored);
    size = (_nMembAvl-_nMembStored)*sizeof(T);

    va_start(arg,format);
    bytesAdded = vsnprintf(dest,size,format,arg);
    va_end(arg);

    // bytesAdded contains the number of bytes that would have
    // been placed in the buffer if the call was successful.
    // this value does not include the trailing null character.
    // so we add one to account for it.

    bytesAdded++;
    nmemb = bytesAdded/sizeof(T);

    if (bytesAdded > size) {
        // we did not fit everything in the original buffer
        // resize and try again.

        // FIXME: round the new size up to the nearest multiple of 256?
        set(_nMembStored+nmemb);

        // reset dest because it may have moved during reallocation
        dest = (char*) (_buf + _nMembStored);
        size = bytesAdded;

        va_start(arg,format);
        bytesAdded = vsnprintf(dest,size,format,arg);
        va_end(arg);

        if (bytesAdded > size) {
            // crystals grow, people grow, data doesn't grow...
            // issue error
            fprintf(stderr,"error in appendf while appending data");
        }
    }

    _nMembStored += nmemb;

    // remove the null character added by vsnprintf()
    // we do this because if we are appending strings,
    // the embedded null acts as a terminating null char.
    // this is a generic buffer so if user wants a
    // terminating null, they should append it.
    remove(1);

    return nmemb;
}

/**
 * Remove bytes from the end of this buffer
 * @param number of bytes to be removed
 * @return number of bytes removed.
 */
template<class T>
int
SimpleBuffer<T>::remove(int nmemb)
{
    if ((_nMembStored - nmemb) < 0){
        _nMembStored = 0;
        _pos = 0;
    } else {
        _nMembStored -= nmemb;
        if (_pos >= _nMembStored) {
            // move _pos back to the new end of the buffer.
            _pos = _nMembStored-1;
        }
    }


    return nmemb;
}


template<class T>
size_t
SimpleBuffer<T>::set(size_t nmemb)
{
    T *buf;
    size_t nbytes = nmemb*sizeof(T);

    if (_buf == NULL) {
        buf = (T*) malloc(nbytes);
    } else {
        buf = (T*) realloc((void*) _buf, nbytes);
    }

    if (buf == NULL) {
        fprintf(stderr,"Can't allocate %lu bytes of memory\n", 
            (long unsigned int)nbytes);
        _fileState = false;
        return 0;
    }
    _buf = buf;
    _nMembAvl = nmemb;
    return _nMembAvl;
}


template<> inline
SimpleBuffer<char>&
SimpleBuffer<char>::show()
{
    size_t curMemb = 0;

    while (curMemb != _nMembStored) {
        fprintf(stdout,"_buf[%lu] = :%c:\n", (long unsigned int)curMemb,
                _buf[curMemb]);
        curMemb += 1;
    }
    fprintf(stdout,"_nMembAvl = :%lu:\n", (long unsigned int)_nMembAvl);

    return *this;
}


template<class T>
SimpleBuffer<T>&
SimpleBuffer<T>::show()
{
    size_t curMemb = 0;

    while (curMemb != _nMembStored) {
        fprintf(stdout,"_buf[%lu] = :%#x:\n", (long unsigned int)curMemb,
                (unsigned long)_buf[curMemb]);
        curMemb += 1;
    }
    fprintf(stdout,"_nMembAvl = :%lu:\n", (long unsigned int)_nMembAvl);

    return *this;
}


/**
 * Read data from the buffer into a memory location provided by caller
 * @param Pointer locating where to place read bytes.
 * @param Size of the memory location.
 * @return Number of bytes written to memory location
 */
template<class T>
size_t
SimpleBuffer<T>::read(const T* bytes, size_t nmemb)
{
    size_t nMembRead = 0;

    void* dest = NULL;
    void const* src  = NULL;
    size_t size = 0;

    // SimpleBuffer is empty.
    if (_buf == NULL) {
        return 0;
    }

    // User specified NULL buffer.
    if (bytes == NULL) {
        return 0;
    }

    // make sure we don't read off the end of our buffer
    if ( (_pos + nmemb) > _nMembStored ) {
        nMembRead = _nMembStored - _pos;
    }
    else {
        nMembRead = nmemb;
    }

    if (nMembRead <= 0) {
        return 0;
    }

    dest = (void*) bytes;
    src  = (void const*) (_buf + _pos);
    size = nMembRead*sizeof(T);
    memcpy(dest,src,size);

    _pos = (_pos + nMembRead);

    return nMembRead;
}


/**
 * Set buffer position indicator to spot within the buffer
 * @param Offset from whence location in buffer.
 * @param Place from where offset is added or subtracted.
 * @return 0 on success, anything else is failure
 */
template<class T>
int
SimpleBuffer<T>::seek(long offset, int whence)
{
    int retVal = 0;

    if (_buf == NULL) {
        return -1 ;
    }

    if (whence == SEEK_SET) {
        if (offset < 0) {
            /* dont go off the beginning of data */
            _pos = 0;
        }
        else if (offset >= (long)_nMembStored) {
            /* dont go off the end of data */
            _pos = _nMembStored - 1;
        }
        else {
            _pos = (size_t)(offset);
        }
    }
    else if (whence == SEEK_CUR) {
        if ( (_pos + offset) < 0) {
            /* dont go off the beginning of data */
            _pos = 0;
        }
        else if ((_pos + offset) >= _nMembStored) {
            /* dont go off the end of data */
            _pos = _nMembStored - 1;
        }
        else {
            _pos = (size_t)(_pos + offset);
        }
    }
    else if (whence == SEEK_END) {
        if (offset <= (long)(-1*_nMembStored)) {
            /* dont go off the beginning of data */
            _pos = 0;
        }
        else if (offset >= 0) {
            /* dont go off the end of data */
            _pos = _nMembStored - 1;
        }
        else {
            _pos = (size_t)((_nMembStored - 1) + offset);
        }
    }
    else {
        retVal = -1;
    }

    return retVal;
}


/**
 * Tell caller the offset of the position indicator from the start of buffer
 * @return Number of bytes the position indicator is from start of buffer
 */
template<class T>
int
SimpleBuffer<T>::tell() const
{
   return (int)_pos;
}


/**
 * Read data from the buffer into a memory location provided by caller
 */
template<class T>
SimpleBuffer<T>&
SimpleBuffer<T>::rewind()
{
    _pos = 0;
    return *this;
}


/**
 * Tell if the last file like operation (ie. read()) was successful
 * or if there was a failure like eof, or bad memory
 * @return True or false boolean value
 */
template<class T>
bool
SimpleBuffer<T>::good() const
{
    return (_fileState);
}


/**
 * Tell if the last file like operation (ie. read()) failed
 * Opposite of good()
 * @return True or false boolean value
 */
template<class T>
bool
SimpleBuffer<T>::bad() const
{
    return (!_fileState);
}


/**
 * Tell if the position flag is at the end of the buffer
 * @return True or false boolean value
 */
template<class T>
bool
SimpleBuffer<T>::eof() const
{
    return (_pos >= _nMembStored);
}


/**
 * Move the data from this SimpleBuffer to the SimpleBuffer provided by
 * the caller. All data except the _pos is moved and this SimpleBuffer is
 * re-initialized with bufferInit().
 * @param SimpleBuffer to move the information to
 * @return reference to this SimpleBuffer object.
 */
template<class T>
SimpleBuffer<T>&
SimpleBuffer<T>::move(SimpleBuffer<T>& b)
{
    bufferFree();

    _buf = b._buf;
    _pos = b._pos;
    _fileState = b._fileState;
    _nMembStored = b._nMembStored;
    _nMembAvl = b._nMembAvl;

    b.bufferInit();

    return *this;
}


 /**
  *  Initializes a dynamic buffer, discarding any previous contents
  *  of the buffer. bufferFree() should have been called already
  *  if the dynamic buffer was previously in use.
  */
template<class T>
void
SimpleBuffer<T>::bufferInit()
{
    _buf = NULL;
    _pos = 0;
    _fileState = true;
    _nMembStored = 0;
    _nMembAvl = 0;
}


/**
 *  Frees up any memory allocated for the dynamic buffer and
 *  reinitializes the buffer to an empty state.
 */
template<class T>
void
SimpleBuffer<T>::bufferFree()
{
    if (_buf != NULL) {
        free(_buf);
        _buf = NULL;
    }
    bufferInit();
}

} // namespace Rappture

#endif // RAPPTURE_SIMPLEBUFFER_H
