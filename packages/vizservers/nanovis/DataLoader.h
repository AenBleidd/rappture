/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef DATA_LOADER_H
#define DATA_LOADER_H

extern void *
LoadDx(const char *fname,
       int& width, int& height, int& depth,
       float& min, float& max, float& nonzero_min, 
       float& axisScaleX, float& axisScaleY, float& axisScaleZ);

extern void *
LoadFlowDx(const char *fname, int& width, int& height, int& depth,
           float& min, float& max, float& nonzero_min, 
           float& axisScaleX, float& axisScaleY, float& axisScaleZ);

extern void *
LoadFlowRaw(const char *fname, int width, int height, int depth,
            float& min, float& max, float& nonzero_min, 
            float& axisScaleX, float& axisScaleY, float& axisScaleZ,
            int skipByte, bool bigEndian);

extern void *
LoadProcessedFlowRaw(const char *fname, int width, int height, int depth,
                     float min, float max,
                     float& axisScaleX, float& axisScaleY, float& axisScaleZ);

extern void
LoadProcessedFlowRaw(const char *fname, int width, int height, int depth,
                     float *data);


#endif
