#ifndef _AXIS_H
#define _AXIS_H

#include "Chain.h"

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
 * TickSweep --
 *
 * 	Structure containing information where step intervalthe ticks (major or
 *	minor) will be displayed on the graph.
 *
 * ----------------------------------------------------------------------
 */
class TickSweep {
private:
    double _initial;		/* Initial value */
    double _step;		/* Size of interval */
    unsigned int _nSteps;	/* Number of intervals. */
public:
    void SetValues(double initial, double step, unsigned int nSteps) {
	_initial = initial, _step = step, _nSteps = nSteps;
    }
    double Initial(void) {
	return _initial;
    }
    double Step(void) {
	return _step;
    }
    unsigned int NumSteps(void) {
	return _nSteps;
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
private:
    int _nTicks;		/* # of ticks on axis */
    float *_ticks;		/* Array of tick values (malloc-ed). */
public:
    Ticks(TickSweep &sweep);
    Ticks(float *ticks, int nTicks) {
	_ticks = ticks, _nTicks = nTicks;
    }
    ~Ticks(void) {
	if (_ticks != NULL) {
	    delete [] _ticks;
	}
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
	AUTOSCALE_MAJOR=(1<<0),
	AUTOSCALE_MINOR=(1<<1),
	DESCENDING=(1<<2),
	LOGSCALE=(1<<3),
	TIGHT_MIN=(1<<4),
	TIGHT_MAX=(1<<5)
    };

    char *_name;		/* Name of the axis. Malloc-ed */

    unsigned int _flags;

    char *_title;		/* Title of the axis. */

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

    double _reqStep;		/* If > 0.0, overrides the computed major 
				 * tick interval.  Otherwise a stepsize 
				 * is automatically calculated, based 
				 * upon the range of elements mapped to the 
				 * axis. The default value is 0.0. */

    Ticks *_t1Ptr;		/* Array of major tick positions. May be
				 * set by the user or generated from the 
				 * major sweep below. */

    Ticks *_t2Ptr;		/* Array of minor tick positions. May be
				 * set by the user or generated from the
				 * minor sweep below. */

    TickSweep _minorSweep, _majorSweep;
    
    Chain _minorTicks, _majorTicks;

    int _reqNumMajorTicks;	/* Default number of ticks to be displayed. */
    int _reqNumMinorTicks;	/* If non-zero, represents the
				 * requested the number of minor ticks
				 * to be uniformally displayed along
				 * each major tick. */

    void LogScale(void);
    void LinearScale(void);
    bool InRange(double x);
    void MakeTicks(void);

public:
    void ResetRange(void);
    void FixRange(double min, double max);
    Axis(const char *name);
    ~Axis(void);
    void SweepTicks(void);
    void SetScale(double min, double max);
    double Scale(void) {
	return _scale;
    }
    double Range(void) {
	return _range;
    }
    bool FirstMinor(TickIter &iter);
    bool FirstMajor(TickIter &iter);
    void GetDataLimits(double min, double max);
    double Map(double x);
    double InvMap(double x);
    char *GetName(void) {
	return _name;
    }
    void SetName(const char *name) {
	_name = new char[strlen(name) + 1];
	strcpy(_name, name);
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
	_reqNumMinorTicks = n;
    }
    void SetNumMajorTicksOption(int n) {
	_reqNumMajorTicks = n;
    }
};



#endif /*_AXIS_H*/
