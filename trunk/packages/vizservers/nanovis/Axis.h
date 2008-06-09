#ifndef _AXIS_H
#define _AXIS_H

#include "Chain.h"

class Axis;

class NaN {
private:
    double _x;
public:
    NaN(void) {
	union DoubleValue {
	    unsigned int words[2];
	    double value;
	} result;
	
#ifdef WORDS_BIGENDIAN
	result.words[0] = 0x7ff80000;
	result.words[1] = 0x00000000;
#else
	result.words[0] = 0x00000000;
	result.words[1] = 0x7ff80000;
#endif
	_x = result.value;
    }
    operator const double() {
	return _x;
    }
};

extern NaN _NaN;

class TickIter {
    ChainLink *_linkPtr;
public:
    void SetStartingLink(ChainLink *linkPtr) {
	_linkPtr = linkPtr;
    }
    bool Next(void) {
	if (_linkPtr == NULL) {
	    return false;
	}
	_linkPtr = _linkPtr->Next();
	return (_linkPtr != NULL);
    }
    float GetValue(void) {
	union {
	    float x;
	    void *clientData;
	} value;
	value.clientData = _linkPtr->GetValue();
	return value.x;
    }
};

/*
 * ----------------------------------------------------------------------
 *
 * Ticks --
 *
 * 	Structure containing information where the ticks (major or
 *	minor) will be displayed on the graph.
 *
 * ----------------------------------------------------------------------
 */
class Ticks {
    bool _autoscale;		/* Indicates if the ticks are autoscaled. */
    /* 
     * Tick locations may be generated in two fashions
     */
    /*	 1. an array of values provided by the user. */
    int _nTicks;		/* # of ticks on axis */
    float *_ticks;		/* Array of tick values (alloc-ed).  Right now
				 * it's a float so we don't have to allocate
				 * store for the list of ticks generated.  In
				 * the future when we store both the tick
				 * label and the value in the list, we may
				 * extend this to a double.
				 */
    /*
     *   2. a defined sweep of initial, step, and nsteps.  This in turn
     *      will generate the an array of values.
     */
    double _initial;		/* Initial value */
    double _step;		/* Size of interval */
    unsigned int _nSteps;	/* Number of intervals. */

    /* 
     *This finally generates a list of ticks values that exclude duplicate
     * minor ticks (that are always represented by major ticks).  In the
     * future, the string representing the tick label will be stored in 
     * the chain.
     */
    Chain _chain;		

    void SetTicks(void);	/* Routine used internally to create the array
				 * of ticks as defined by a given sweep. */
    void *GetClientData(float x) {
	union {
	    float x;
	    void *clientData;
	} value;
	value.x = x;
	return value.clientData;
    }

public:
    int reqNumTicks;		/* Default number of ticks to be displayed. */

    Ticks(int numTicks) {
	reqNumTicks = numTicks;
	_ticks = NULL;
	_nTicks = 0;
	_autoscale = true;
    }
    ~Ticks(void) {
	if (_ticks != NULL) {
	    delete [] _ticks;
	}
	_chain.Reset();
	
    }
    void SetTicks(float *ticks, int nTicks) {
	_ticks = ticks, _nTicks = nTicks;
    }
    int NumTicks(void) {
	return _nTicks;
    }
    double GetTick(int i) {
	if ((i < 0) || (i >= _nTicks)) {
	    return _NaN;
	}
	return _ticks[i];
    }
    void Reset(void) {
	_chain.Reset();
    }
    float Step() {
	return _step;
    }
    void Append (float x) {
	_chain.Append(GetClientData(x));
    }    
    void SetValues(double initial, double step, unsigned int nSteps) {
	_initial = initial, _step = step, _nSteps = nSteps;
    }
    bool IsAutoScale(void) {
	return _autoscale;
    }
    void SweepTicks(void) {
	if (_autoscale) {
	    if (_ticks != NULL) {
		delete [] _ticks;
	    }
	    SetTicks();
	}
    }
    bool FirstTick(TickIter &iter) {
	ChainLink *linkPtr;

	linkPtr = _chain.FirstLink();
	iter.SetStartingLink(linkPtr);
	return (linkPtr != NULL);
    }
};


/*
 * ----------------------------------------------------------------------
 *
 * Axis --
 *
 * 	Structure contains options controlling how the axis will be
 * 	displayed.
 *
 * ----------------------------------------------------------------------
 */
class Axis {
    /* Flags associated with the axis. */
    enum AxisFlags {
	AUTOSCALE=(1<<0),
	DESCENDING=(1<<1),
	LOGSCALE=(1<<2),
	TIGHT_MIN=(1<<3),
	TIGHT_MAX=(1<<4)
    };

    const char *_name;		/* Name of the axis. Malloc-ed */

    unsigned int _flags;

    const char *_title;		/* Title of the axis. */

    const char *_units;		/* Units of the axis. */

    double _valueMin, _valueMax; /* The limits of the data. */

    double _reqMin, _reqMax;	/* Requested axis bounds. Consult the
				 * axisPtr->flags field for
				 * Axis::CONFIG_MIN and Axis::CONFIG_MAX
				 * to see if the requested bound have
				 * been set.  They override the
				 * computed range of the axis
				 * (determined by auto-scaling). */

    double _min, _max;		/* Smallest and largest major tick values for
				 * the axis.  If loose, the tick values lie
				 * outside the range of data values.  If
				 * tight, they lie interior to the limits of
				 * the data. */

    double _range;		/* Range of values on axis (_max - _min) */
    double _scale;		/* Scale factor for axis (1.0/_range) */

    double _reqStep;		/* If > 0.0, overrides the computed major tick
				 * interval.  Otherwise a stepsize is
				 * automatically calculated, based upon the
				 * range of elements mapped to the axis. The
				 * default value is 0.0. */

    Ticks _major, _minor;

    void LogScale(void);
    void LinearScale(void);
    bool InRange(double x);
    void MakeTicks(void);

public:
    Axis(const char *name);
    ~Axis(void) {
	if (_name != NULL) {
	    free((void *)_name);
	}
	if (_units != NULL) {
	    free((void *)_units);
	}
	if (_title != NULL) {
	    free((void *)_title);
	}
    }

    void ResetRange(void);
    void FixRange(double min, double max);
    void SetScale(double min, double max);
    double Scale(void) {
	return _scale;
    }
    double Range(void) {
	return _range;
    }
    bool FirstMajor(TickIter &iter) {
	return _major.FirstTick(iter);
    }
    bool FirstMinor(TickIter &iter) {
	return _minor.FirstTick(iter);
    }
    void GetDataLimits(double &min, double &max) {
	min = _valueMin, max = _valueMax;
    }
    double Map(double x);
    double InvMap(double x);
    const char *GetName(void) {
	return (_name == NULL) ? "???" : _name;
    }
    void SetName(const char *name) {
	if (_name != NULL) {
	    free((void *)_name);
	}
	_name = strdup(name);
    }
    const char *GetUnits(void) {
	return (_units == NULL) ? "???" : _units;
    }
    void SetUnits(const char *units) {
	if (_units != NULL) {
	    free((void *)_units);
	}
	_units = strdup(units);
    }
    const char *GetTitle(void) {
	return (_title == NULL) ? "???" : _title;
    }
    void SetTitle(const char *title) {
	if (_title != NULL) {
	    free((void *)_title);
	}
	_title = strdup(title);
    }
    void SetMin(double min) {
	_reqMin = min;		// Setting to NaN resets min to "auto"
    }
    void SetMax(double min) {
	_reqMin = min;		// Setting to NaN resets max to "auto"
    }
    void SetLimits(double min, double max) {
	SetMin(min), SetMax(max);
    }
    void UnsetLimits() {
	SetMin(_NaN), SetMax(_NaN);
    }
    void SetDescendingOption(bool value) {
	if (value) {
	    _flags |= DESCENDING;
	} else {
	    _flags &= ~DESCENDING;
	}
    }
    void SetTightMinOption(bool value) {
	if (value) {
	    _flags |= TIGHT_MIN;
	} else {
	    _flags &= ~TIGHT_MIN;
	}
    }
    void SetTightMaxOption(bool value) {
	if (value) {
	    _flags |= TIGHT_MAX;
	} else {
	    _flags &= ~TIGHT_MAX;
	}
    }
    void SetLogScaleOption(bool value) {
	if (value) {
	    _flags |= LOGSCALE;
	} else {
	    _flags &= ~LOGSCALE;
	}
    }
    void SetMajorStepOption(double value) {
	_reqStep = value;	// Setting to 0.0 resets step to "auto"
    }
    void SetNumMinorTicksOption(int n) {
	_minor.reqNumTicks = n;
    }
    void SetNumMajorTicksOption(int n) {
	_major.reqNumTicks = n;
    }
};

#endif /*_AXIS_H*/
