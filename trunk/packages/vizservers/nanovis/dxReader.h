/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef DXREADER_H
#define DXREADER_H

#include <iostream>

namespace Rappture {
    class Outcome;
}
class Volume;

extern Volume *
load_volume_stream(Rappture::Outcome& status, const char *tag, 
                   std::iostream& fin);

#endif
