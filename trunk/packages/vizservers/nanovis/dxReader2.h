/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef DXREADER2_H
#define DXREADER2_H

namespace Rappture {
    class Outcome;
}
class Volume;

extern Volume *
load_volume_stream_odx(Rappture::Outcome& status, 
                       const char *tag, const char *buf, int nBytes);

#endif
