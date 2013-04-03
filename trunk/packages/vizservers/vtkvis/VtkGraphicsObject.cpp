/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include "VtkGraphicsObject.h"
#include "VtkRenderer.h"
#include "Trace.h"

using namespace VtkVis;

void VtkGraphicsObject::setDataSet(DataSet *dataSet,
                                   Renderer *renderer)
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;

        if (renderer != NULL &&
            dataSet != NULL &&
            renderer->getUseCumulativeRange()) {
            renderer->getCumulativeDataRange(_dataRange,
                                             _dataSet->getActiveScalarsName(),
                                             1);
        } else if (_dataSet != NULL) {
            _dataSet->getScalarRange(_dataRange);
        }

        update();
    }
}

void VtkGraphicsObject::updateRanges(Renderer *renderer)
{
    if (_dataSet == NULL) {
        ERROR("called before setDataSet");
        return;
    }
    if (renderer == NULL) {
        ERROR("NULL renderer");
        return;
    }

    if (renderer->getUseCumulativeRange()) {
        renderer->getCumulativeDataRange(_dataRange,
                                         _dataSet->getActiveScalarsName(),
                                         1);
    } else {
        _dataSet->getScalarRange(_dataRange);
    }
}
