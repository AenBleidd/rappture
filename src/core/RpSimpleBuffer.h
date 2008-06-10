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
 *  in the software and related documentation as defined in the Federal�
 *  Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 *  are acquiring the software on behalf of the Department of Defense, the
 *  software shall be classified as "Commercial Computer Software" and the
 *  Government shall have only "Restricted Rights" as defined in Clause
 *  252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
 *  authors grant the U.S. Government and others acting in its behalf
 *  permission to use and distribute the software in accordance with the
 *  terms specified in this license.�
 *
 * ======================================================================
 */

#ifndef RAPPTURE_SIMPLEBUFFER_H
#define RAPPTURE_SIMPLEBUFFER_H

#include <fstream>

#define RP_SIMPLEBUFFER_MIN_SIZE    256

#ifdef __cplusplus
    extern "C" {
#endif // ifdef __cplusplus


namespace Rappture {

class SimpleBuffer {
public:
    SimpleBuffer();
    SimpleBuffer(int nbytes);
    SimpleBuffer(const char* bytes, int nbytes=-1);
    SimpleBuffer(const SimpleBuffer& b);
    SimpleBuffer& operator=(const SimpleBuffer& b);
    SimpleBuffer  operator+(const SimpleBuffer& b) const;
    SimpleBuffer& operator+=(const SimpleBuffer& b);
    virtual ~SimpleBuffer();

    const char* bytes() const;
    unsigned int size() const;

    SimpleBuffer& clear();
    int append(const char* bytes, int nbytes=-1);
    int read(const char* bytes, int nbytes);
    int seek(int offset, int whence);
    int tell() const;
    SimpleBuffer& rewind();

    bool good() const;
    bool bad() const;
    bool eof() const;

    SimpleBuffer& move(SimpleBuffer& b);

protected:

    void bufferInit();
    void bufferFree();

private:

    /// Pointer to the memory that holds our buffer's data
    char* _buf;

    /// Position offset within the buffer's memory
    unsigned int _pos;

    /// Size of the used memory in the buffer
    unsigned int _size;

    /// Total space available in the buffer
    unsigned int _spaceAvl;

    /// State of the last file like operation.
    bool _fileState;
};

} // namespace Rappture
 
#ifdef __cplusplus
    }
#endif // ifdef __cplusplus

#endif // RAPPTURE_SIMPLEBUFFER_H
