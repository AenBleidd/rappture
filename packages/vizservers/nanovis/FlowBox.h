/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Authors:
 *   Wei Qiao <qiaow@purdue.edu>
 *   Insoo Woo <iwoo@purdue.edu>
 *   George A. Howlett <gah@purdue.edu>
 *   Leif Delgass <ldelgass@purdue.edu>
 */
#ifndef FLOWBOX_H
#define FLOWBOX_H

#include <string>

#include <tcl.h>

#include <vrmath/Vector3f.h>

#include "FlowTypes.h"
#include "Switch.h"
#include "Volume.h"

namespace nv {

struct FlowBoxValues {
    FlowPoint corner1, corner2;    ///< Coordinates of the box.
    FlowColor color;               ///< Color of box
    float lineWidth;
    int isHidden;
};

class FlowBox
{
public:
    FlowBox(const char *name);
    ~FlowBox();

    const char *name() const
    {
        return _name.c_str();
    }

    bool visible() const
    {
        return !_sv.isHidden;
    }

    int parseSwitches(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
    {
        if (Rappture::ParseSwitches(interp, _switches, objc, objv, &_sv, 
                                    SWITCH_DEFAULTS) < 0) {
            return TCL_ERROR;
        }
        return TCL_OK;
    }

    void render(Volume *volume);

    const FlowBoxValues *getValues() const
    {
        return &_sv;
    }

    void getWorldSpaceBounds(vrmath::Vector3f& min,
                             vrmath::Vector3f& max,
                             const Volume *volume) const;

private:
    std::string _name;          ///< Name of this box in the hash table.
    FlowBoxValues _sv;
    static Rappture::SwitchSpec _switches[];
};

}

#endif
