/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef NV_VTKDATASET_READER_H
#define NV_VTKDATASET_READER_H

#include <iostream>

namespace nv {

class Volume;

extern Volume *
load_vtk_volume_stream(const char *tag, const char *bytes, int nBytes);

}

#endif
