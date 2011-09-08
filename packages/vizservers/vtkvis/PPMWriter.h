/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_PPMWRITER_H__
#define __RAPPTURE_VTKVIS_PPMWRITER_H__

namespace Rappture {
namespace VtkVis {

extern
void writePPM(int fd, const char *cmdName, const unsigned char *data, int width, int height);

}
}

#endif
