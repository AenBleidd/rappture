/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>
#include <iostream>
#include <fstream>

// rappture headers
#include <RpEncode.h>
#include <RpOutcome.h>

#include "nvconf.h"
#ifdef HAVE_DX_DX_H
#include "RpDX.h"
#undef ERROR

#include "nanovis.h"
#include "dxReaderCommon.h"
#include "Volume.h"

/* Load a 3D volume from a dx-format file the new way
 */
Volume *
load_volume_stream_odx(Rappture::Outcome& context, const char *tag, 
                       const char *buf, int nBytes)
{
    char dxfilename[128];

    if (nBytes <= 0) {
	context.error("empty data buffer\n");
        return NULL;
    }

    // write the dx file to disk, because DXImportDX takes a file name
    sprintf(dxfilename, "/tmp/dx%d.dx", getpid());

    FILE *f;

    f = fopen(dxfilename, "w");

    ssize_t nWritten;
    nWritten = fwrite(buf, sizeof(char), nBytes, f);
    fclose(f);
    if (nWritten != nBytes) {
        context.addError("Can't read %d bytes from file \"%s\"\n", 
			 nBytes, dxfilename);
	return NULL;
    }

    Rappture::DX dxObj(context, dxfilename);

    if (unlink(dxfilename) != 0) {
        context.addError("Error deleting dx file: %s\n", dxfilename);
	return NULL;
    }

    int nx = dxObj.axisLen()[0];
    int ny = dxObj.axisLen()[1];
    int nz = dxObj.axisLen()[2];
    float dx = nx;
    float dy = ny;
    float dz = nz;

    const float *data1 = dxObj.data();
    float *data = new float[nx*ny*nz*4];
    memset(data, 0, nx*ny*nz*4);
    int iz = 0, ix = 0, iy = 0;
    float dv = dxObj.dataMax() - dxObj.dataMin();
    float vmin = dxObj.dataMin();

    for (int i = 0; i < nx*ny*nz; i++) {
        int nindex = (iz*nx*ny + iy*nx + ix) * 4;
        float v = data1[i];

        // scale all values [0-1], -1 => out of bounds
        v = (isnan(v)) ? -1.0 : (v - vmin)/dv;

        // place the value in the correct index in cdata
        data[nindex] = v;

        // calculate next x,y,z coordinate
        if (++iz >= nz) {
            iz = 0;
            if (++iy >= ny) {
                iy = 0;
                ++ix;
            }
        }
    }

    computeSimpleGradient(data, nx, ny, nz);

    TRACE("nx = %i ny = %i nz = %i\n", nx, ny, nz);
    TRACE("dx = %lg dy = %lg dz = %lg\n", dx, dy, dz);
    TRACE("dataMin = %lg\tdataMax = %lg\tnzero_min = %lg\n",
          dxObj.dataMin(), dxObj.dataMax(), dxObj.nzero_min());

    Volume *volPtr;
    volPtr = NanoVis::load_volume(tag, nx, ny, nz, 4, data, 
				  dxObj.dataMin(), 
				  dxObj.dataMax(), 
				  dxObj.nzero_min());
    const float *origin = dxObj.origin();
    const float *max = dxObj.max();

    volPtr->xAxis.SetRange(origin[0], max[0]);
    volPtr->yAxis.SetRange(origin[1], max[1]);
    volPtr->zAxis.SetRange(origin[2], max[2]);
    volPtr->wAxis.SetRange(dxObj.dataMin(), dxObj.dataMax());
    volPtr->update_pending = true;

    delete [] data; 

    //
    // Center this new volume on the origin.
    //
    float dx0 = -0.5;
    float dy0 = -0.5*dy/dx;
    float dz0 = -0.5*dz/dx;
    volPtr->location(Vector3(dx0, dy0, dz0));
    return volPtr;
}
#endif
