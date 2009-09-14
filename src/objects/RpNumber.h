/*
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <errno.h>
#include "RpObject.h"
#include "RpChain.h"

#ifndef RAPPTURE_NUMBER_H
#define RAPPTURE_NUMBER_H

namespace Rappture {

class Number : public Object
{
    public:

        Number();
        Number(const char *name, const char *units, double val);

        Number(const char *name, const char *units, double val,
               double min, double max, const char *label,
               const char *desc);

        Number( const Number& o );
        virtual ~Number ();

        Accessor<double> def;
        Accessor<double> cur;
        Accessor<double> min;
        Accessor<double> max;

        const char *units(void) const;
        void units(const char *p);

        // convert the value stored in this object to specified units
        // does not return the converted value
        // error code is returned
        int convert(const char *to);

        // get the value of this object converted to specified units
        // does not change the value of the object
        // error code is returned
        int value(const char *units, double *value) const;

        Number& addPreset(const char *label, const char *desc,
                          double val, const char *units);

        Number& addPreset(const char *label, const char *desc,
                          const char *val);

        Number& delPreset(const char *label);


        void configure(size_t as, ClientData p);
        void dump(size_t as, ClientData p);
        // const char* xml(size_t indent, size_t tabstop);

        const int is() const;

        void minFromStr(const char *val);
        void maxFromStr(const char *val);
        void defFromStr(const char *val);
        void curFromStr(const char *val);

    private:

        // flag tells if user specified min and max values
        int _minSet;
        int _maxSet;

        // hash or linked list of preset values
        Rp_Chain *_presets;

        struct preset{
            Accessor<const char *> label;
            Accessor<const char *> desc;
            Accessor<const char *> units;
            Accessor<double> val;
        };

        void __configureFromXml(const char *p);
        void __configureFromTree(Rp_ParserXml *p);
        void __dumpToXml(ClientData c);
        void __dumpToTree(ClientData c);
};

} // namespace Rappture

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
