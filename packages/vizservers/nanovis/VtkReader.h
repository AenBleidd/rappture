/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef NV_VTKREADER_H
#define NV_VTKREADER_H

#include <iostream>

namespace Rappture {
class Outcome;
}

namespace nv {

class Volume;

extern Volume *
load_vtk_volume_stream(Rappture::Outcome& status, const char *tag, std::iostream& fin);

}

#endif
