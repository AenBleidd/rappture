/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_TGAWRITER_H__
#define __RAPPTURE_VTKVIS_TGAWRITER_H__

namespace Rappture {
namespace VtkVis {

extern
void writeTGA(int fd, const char *cmdName, const unsigned char *data, int width, int height);

extern
void writeTGAFile(const char *filename, const unsigned char *data, int width, int height);

}
}

#endif
