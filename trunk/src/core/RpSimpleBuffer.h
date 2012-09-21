
/*
 * ======================================================================
 *  Rappture::SimpleBuffer
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
 *  in the software and related documentation as defined in the Federal
 *  Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 *  are acquiring the software on behalf of the Department of Defense, the
 *  software shall be classified as "Commercial Computer Software" and the
 *  Government shall have only "Restricted Rights" as defined in Clause
 *  252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
 *  authors grant the U.S. Government and others acting in its behalf
 *  permission to use and distribute the software in accordance with the
 *  terms specified in this license.
 *
 * ======================================================================
 */

#ifndef RAPPTURE_SIMPLEBUFFER_H
#define RAPPTURE_SIMPLEBUFFER_H

#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cstdarg>

/* 
 * Note: I think I would redo this class to deal only with unsigned byte 
 *	 arrays, instead of arrays of <T>.  I would create other classes
 *	 that arrays of <T> that used the lower level byte array.
 *
 *	 It would make it cleaner if the class only dealt with bytes
 *	 instead of elements (of various sizes).  
 *
 *	 Specific implementations of <T> could perform data alignment on
 *	 word/double word/quad word boundaries.  This is an optimization
 *	 that should could done for double arrays.
 *
 * Note: The signed int argument on append() should be replaced with a 
 *	 size_t.  This limits the length of strings to appended.  It 
 *	 silently truncates bigger sizes to the lower 32-bits.  
 */
namespace Rappture {

template <class T>
class SimpleBuffer {
public:
    /**
     * Construct an empty SimpleBuffer.
     */
    SimpleBuffer() {
	Initialize();
    }
    /**
     * Construct a SimpleBuffer loaded with initial data.
     *
     * @param bytes pointer to bytes being stored.
     * @param nbytes number of bytes being stored.
     */
    SimpleBuffer(const T* bytes, int numElems=-1) {
	Initialize();
	append(bytes, numElems);
    }
    SimpleBuffer(size_t nmemb);
    
    /**
     * Copy constructor
     * @param SimpleBuffer object to copy
     */
    SimpleBuffer(const SimpleBuffer& b);
    
    SimpleBuffer<T>& operator=(const SimpleBuffer<T>& b);
    SimpleBuffer     operator+(const SimpleBuffer& b) const;
    SimpleBuffer<T>& operator+=(const SimpleBuffer<T>& b);
    T operator[](size_t offset);
    
    /**
     * Destructor
     */
    virtual ~SimpleBuffer() {
	Release();
    }
    
    /**
     * Get the bytes currently stored in the buffer.  These bytes can
     * be stored, and used later to construct another Buffer to
     * decode the information.
     *
     * @return Pointer to the bytes in the buffer.
     */
    const T* bytes() const {
	return _buf;
    }
    /**
     * Get the number of bytes currently stored in the buffer.
     * @return Number of the bytes used in the buffer.
     */
    size_t size() const {
	return _numElemsUsed * sizeof(T);
    }
    /**
     * Get the number of members currently stored in the buffer.
     * @return Number of the members used in the buffer.
     */
    size_t nmemb() const {
	return _numElemsUsed;
    }
    /**
     * Get the number of members currently stored in the buffer.
     * @return Number of the members used in the buffer.
     */
    size_t count() const {
	return _numElemsUsed;
    }
    /**
     * Set the number of members currently stored in the buffer.
     * @return Number of the members used in the buffer.
     */
    size_t count(size_t newCount) {
	_numElemsUsed = newCount;
	return _numElemsUsed;
    }
    /**
     * Clear the buffer, making it empty.
     * @return Reference to this buffer.
     */
    SimpleBuffer<T>& clear() {
	Release();
	return *this;
    }
    size_t extend(size_t extraElems);
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
    
    void Initialize();
    void Release();
    
private:
    
    /// Pointer to the memory that holds our buffer's data
    T* _buf;
    
    /// Position offset within the buffer's memory
    size_t _pos;
    
    /// Number of members stored in the buffer
    size_t _numElemsUsed;
    
    /// Total number of members available in the buffer
    size_t _numElemsAllocated;
    
    /// State of the last file like operation.
    bool _fileState;
    
    /// Minimum number of members is set to the number you can fit in 256 bytes
    const static size_t _minNumElems = (256/sizeof(T));
    
    size_t __guesslen(const T* bytes);
    
};
    
typedef SimpleBuffer<char>   SimpleCharBuffer;
typedef SimpleBuffer<float>  SimpleFloatBuffer;
typedef SimpleBuffer<double> SimpleDoubleBuffer;

/**
 * Construct an empty SimpleBuffer of specified size.
 */
template<class T>
SimpleBuffer<T>::SimpleBuffer(size_t numElems)
{
    Initialize();

    if (numElems == 0) {
        // ignore requests for sizes equal to zero
        return;
    }

    // buffer sizes less than min_size are set to min_size
    if (numElems < (size_t) _minNumElems) {
        numElems = _minNumElems;
    }

    if (set(numElems) != numElems) {
        return;
    }
    _numElemsUsed = numElems;
}


/**
 * Copy constructor
 * @param SimpleBuffer object to copy
 */
template<class T>
SimpleBuffer<T>::SimpleBuffer(const SimpleBuffer<T>& b)
{
    Initialize();
    append(b.bytes(), b.nmemb());
}


/**
 * Assignment operator
 * @param SimpleBuffer object to copy
 */
template<class T>
SimpleBuffer<T>&
SimpleBuffer<T>::operator=(const SimpleBuffer<T>& b)
{
    Release();
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
 * Operator []
 * @param index into the buffer
 */
template<class T>
T
SimpleBuffer<T>::operator[](size_t index)
{
    return (_buf + index);		// Rely on pointer arithmetic
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

/* FIXME:	Change the signed int to size_t.  Move the -1 copy string
 *		up to the SimpleCharBuffer class.   There needs to be both
 *		a heavy duty class that can take a string bigger than 
 *		2^31-1 in length, and a convenient class that doesn't make
 *		you add call strlen(s).
 *		
 */

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
SimpleBuffer<T>::append(const T* bytes, int numElems)
{

    // User specified NULL buffer to append
    if (bytes == NULL) {
        return 0;			// Should not give append a NULL
					// buffer pointer.
    }

    // FIXME: i think this needs to be division,
    // need to create test case with array of ints
    // i'm not sure we can even guess the number
    // bytes in *bytes.

    size_t numBytes = 0;
    if (numElems == -1) {
	/* This should be moved into the SimpleCharBuffer.  It doesn't make
	 * sense to look for a NUL byte unless it's a string buffer. We
	 * can then change numElems to be size_t. */
        numBytes = __guesslen(bytes);
        numElems = numBytes / sizeof(T);
    }
    if (numElems <= 0) {
        return numElems;
    }

    size_t newSize;
    newSize = _numElemsUsed + numElems;
    if (newSize > _numElemsAllocated) {

        // buffer sizes less than min_size are set to min_size
        if (newSize < _minNumElems) {
            newSize = _minNumElems;
        }

        /*
         * Allocate a larger buffer for the string if the current one isn't
         * large enough. Allocate extra space in the new buffer so that there
         * will be room to grow before we have to allocate again.
         */
        size_t size;
        size = (_numElemsAllocated > 0) ? _numElemsAllocated : _minNumElems;
        while (newSize > size) {
            size += size;
        }

        /*
         * reallocate to a larger buffer
         */
        if (set(size) != size) {
            return 0;
        }
    }
    memcpy(_buf + _numElemsUsed, bytes, numElems * sizeof(T));
    _numElemsUsed += numElems;
    return numElems;
}

/**
 * Append bytes to the end of this buffer
 * @param pointer to bytes to be added
 * @param number of bytes to be added
 * @return number of bytes appended.
 */
template<class T>
size_t
SimpleBuffer<T>::extend(size_t numExtraElems)
{
    size_t newSize;

    newSize = _numElemsUsed + numExtraElems;
    if (newSize > _numElemsAllocated) {

        /* Enforce a minimum buffer size. */
        if (newSize < (size_t) _minNumElems) {
            newSize = (size_t) _minNumElems;
        }

        size_t size;
        size = (_numElemsAllocated > 0) ? _numElemsAllocated : _minNumElems;

	/* Keep doubling the size of the buffer until we have enough space to
	 * hold the extra elements. */
        while (newSize > size) {
            size += size;
        }
        /* Reallocate to a larger buffer. */
        if (set(size) != size) {
            return 0;
        }
    }
    return _numElemsAllocated;
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

    newMembCnt = (size_t)(_numElemsUsed + nmemb);

    if (newMembCnt > _numElemsAllocated) {

        // buffer sizes less than min_size are set to min_size
        if (newMembCnt < (size_t) _minNumElems) {
            newMembCnt = (size_t) _minNumElems;
        }

        /*
         * Allocate a larger buffer for the string if the current one isn't
         * large enough. Allocate extra space in the new buffer so that there
         * will be room to grow before we have to allocate again.
         */
        size_t membAvl;
        membAvl = (_numElemsAllocated > 0) ? _numElemsAllocated : _minNumElems;
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

    dest = (char*) (_buf + _numElemsUsed);
    size = (_numElemsAllocated-_numElemsUsed)*sizeof(T);

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
        set(_numElemsUsed+nmemb);

        // reset dest because it may have moved during reallocation
        dest = (char*) (_buf + _numElemsUsed);
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

    _numElemsUsed += nmemb;

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
    if ((_numElemsUsed - nmemb) < 0){
        _numElemsUsed = 0;
        _pos = 0;
    } else {
        _numElemsUsed -= nmemb;
        if (_pos >= _numElemsUsed) {
            // move _pos back to the new end of the buffer.
            _pos = _numElemsUsed-1;
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
    _numElemsAllocated = nmemb;
    return _numElemsAllocated;
}

template<> inline
SimpleBuffer<char>&
SimpleBuffer<char>::show()
{
    size_t curMemb = 0;

    while (curMemb != _numElemsUsed) {
        fprintf(stdout,"_buf[%lu] = :%c:\n", (long unsigned int)curMemb,
                _buf[curMemb]);
        curMemb += 1;
    }
    fprintf(stdout,"_numElemsAllocated = :%lu:\n", 
	    (long unsigned int)_numElemsAllocated);

    return *this;
}


template<class T>
SimpleBuffer<T>&
SimpleBuffer<T>::show()
{
    size_t curMemb = 0;

    while (curMemb != _numElemsUsed) {
        fprintf(stdout,"_buf[%lu] = :%#x:\n", (long unsigned int)curMemb,
                (unsigned long)_buf[curMemb]);
        curMemb += 1;
    }
    fprintf(stdout,"_numElemsAllocated = :%lu:\n", (long unsigned int)_numElemsAllocated);

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
    if ( (_pos + nmemb) > _numElemsUsed ) {
        nMembRead = _numElemsUsed - _pos;
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
        else if (offset >= (long)_numElemsUsed) {
            /* dont go off the end of data */
            _pos = _numElemsUsed - 1;
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
        else if ((_pos + offset) >= _numElemsUsed) {
            /* dont go off the end of data */
            _pos = _numElemsUsed - 1;
        }
        else {
            _pos = (size_t)(_pos + offset);
        }
    }
    else if (whence == SEEK_END) {
        if (offset <= (long)(-1*_numElemsUsed)) {
            /* dont go off the beginning of data */
            _pos = 0;
        }
        else if (offset >= 0) {
            /* dont go off the end of data */
            _pos = _numElemsUsed - 1;
        }
        else {
            _pos = (size_t)((_numElemsUsed - 1) + offset);
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
 * Move the internal position tracker to the beginning of the buffer.
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
    return (_pos >= _numElemsUsed);
}


/**
 * Move the data from this SimpleBuffer to the SimpleBuffer provided by
 * the caller. All data except the _pos is moved and this SimpleBuffer is
 * re-initialized with Initialize().
 * @param SimpleBuffer to move the information to
 * @return reference to this SimpleBuffer object.
 */
template<class T>
SimpleBuffer<T>&
SimpleBuffer<T>::move(SimpleBuffer<T>& b)
{
    Release();

    _buf = b._buf;
    _pos = b._pos;
    _fileState = b._fileState;
    _numElemsUsed = b._numElemsUsed;
    _numElemsAllocated = b._numElemsAllocated;

    b.Initialize();

    return *this;
}


 /**
  *  Initializes a dynamic buffer, discarding any previous contents
  *  of the buffer. Release() should have been called already
  *  if the dynamic buffer was previously in use.
  */
template<class T>
void
SimpleBuffer<T>::Initialize()
{
    _buf = NULL;
    _pos = 0;
    _fileState = true;
    _numElemsUsed = 0;
    _numElemsAllocated = 0;
}


/**
 *  Frees up any memory allocated for the dynamic buffer and
 *  reinitializes the buffer to an empty state.
 */
template<class T>
void
SimpleBuffer<T>::Release()
{
    if (_buf != NULL) {
        free(_buf);
        _buf = NULL;
    }
    Initialize();
}

} // namespace Rappture

#endif // RAPPTURE_SIMPLEBUFFER_H
