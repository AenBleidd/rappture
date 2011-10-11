/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include "RpVtkGraphicsObject.h"
#include "RpVtkRenderer.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

void VtkGraphicsObject::setDataSet(DataSet *dataSet,
                                   Renderer *renderer)
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;

        if (renderer->getUseCumulativeRange()) {
            renderer->getCumulativeDataRange(_dataRange,
                                             _dataSet->getActiveScalarsName(),
                                             1);
        } else {
            _dataSet->getScalarRange(_dataRange);
        }

        update();
    }
}

void VtkGraphicsObject::updateRanges(Renderer *renderer)
{
    if (renderer->getUseCumulativeRange()) {
        renderer->getCumulativeDataRange(_dataRange,
                                         _dataSet->getActiveScalarsName(),
                                         1);
    } else if (_dataSet != NULL) {
        _dataSet->getScalarRange(_dataRange);
    } else {
        WARN("updateRanges called before setDataSet");
    }
}
