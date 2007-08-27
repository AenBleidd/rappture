/**
 * ----------------------------------------------------------------------
 * @class NvZincBlendeReconstructor 
 * 
 * ======================================================================
 * @author Insoo Woo (iwoo@purdue.edu)
 * @author Purdue Rendering and Perceptualization Lab (PURPL)
 * 
 * Copyright (c) 2004-2007  Purdue Research Foundation
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef __NV_ZINC_BLENDE_RECONSTRUCTOR_H__
#define __NV_ZINC_BLENDE_RECONSTRUCTOR_H__

#include <stdio.h>
#include <ostream>
#include <sstream>
#include <fstream>

#include "Vector3.h"

class ZincBlendeVolume;

class NvZincBlendeReconstructor {
    char buff[255];

    /**
     * @brief A ZincBlendeReconstructor Singleton instance
     */
    static NvZincBlendeReconstructor* _instance;
private :
    /**
     * @brief Constructor
     */
    NvZincBlendeReconstructor();

    /**
     * @brief Destructor
     */
    ~NvZincBlendeReconstructor();

public :
    /**
     * @brief Return a singleton instance
     */
    static NvZincBlendeReconstructor* getInstance();

private :
    /**
     * @brief Get a line from file. It is used for reading header because header is written in ascii.
     * @param fp A file pointer of data file
     */
    void getLine(std::istream& stream);
    
public :
    /**
     * @brief Load Zinc blende binary volume data and create ZincBlendeVolume with the file
     * @param fileName Zinc blende file name, which data is generated by NEMO-3D
     */
    ZincBlendeVolume* loadFromFile(const char* fileName);

    /**
     * @brief Load Zinc blende binary volume data and create ZincBlendeVolume with the file
     * @param data Zinc blende binary volume data, which data is generated by NEMO-3D and transferred from nanoscale
     */
    ZincBlendeVolume* loadFromStream(std::istream& stream);

    /**
     * @brief Create ZincBlendVolume with output data of NEMO-3D
     * @param origin the start position of the volume data
     * @param delta  the delta value among atoms
     * @param width  the width of unit cells in the data
     * @param height the height of unit cells in the data
     * @param depth  the depth of unit cells in the data
     * @param data the memory block of output data of NEMO-3D
     */
    ZincBlendeVolume* buildUp(const Vector3& origin, const Vector3& delta, int width, int height, int depth, void* data);
    ZincBlendeVolume* buildUp(const Vector3& origin, const Vector3& delta, int width, int height, int depth, int datacount, double emptyvalue, void* data);
};

#endif

