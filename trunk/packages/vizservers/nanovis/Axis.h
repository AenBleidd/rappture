/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef AXIS_H
#define AXIS_H

#include "Chain.h"

class Axis;

class NaN
{
private:
    double _x;
public:
    NaN()
    {
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

    operator double() const
    {
	return _x;
    }
};

extern NaN _NaN;

class TickIter {
    ChainLink *linkPtr_;
public:
    void SetStartingLink(ChainLink *linkPtr) {
	linkPtr_ = linkPtr;
    }
    bool Next(void) {
	if (linkPtr_ == NULL) {
	    return false;
	}
	linkPtr_ = linkPtr_->Next();
	return (linkPtr_ != NULL);
    }
    float GetValue(void) {
	union {
	    float x;
	    void *clientData;
	} value;
	value.clientData = linkPtr_->GetValue();
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
    bool autoscale_;		/* Indicates if the ticks are autoscaled. */
    /* 
     * Tick locations may be generated in two fashions
     */
    /*	 1. an array of values provided by the user. */
    int numTicks_;		/* # of ticks on axis */
    float *ticks_;		/* Array of tick values (alloc-ed).  Right now
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
    double initial_;		/* Initial value */
    double step_;		/* Size of interval */
    unsigned int nSteps_;	/* Number of intervals. */

    /* 
     *This finally generates a list of ticks values that exclude duplicate
     * minor ticks (that are always represented by major ticks).  In the
     * future, the string representing the tick label will be stored in 
     * the chain.
     */
    Chain chain_;		

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
	ticks_ = NULL;
	numTicks_ = 0;
	autoscale_ = true;
    }
    ~Ticks(void) {
	if (ticks_ != NULL) {
	    delete [] ticks_;
	}
	chain_.Reset();
	
    }
    void SetTicks(float *ticks, int nTicks) {
	ticks_ = ticks, numTicks_ = nTicks;
    }
    int numTicks(void) {
	return numTicks_;
    }
    double tick(int i) {
	if ((i < 0) || (i >= numTicks_)) {
	    return _NaN;
	}
	return ticks_[i];
    }
    void Reset(void) {
	chain_.Reset();
    }
    float step() {
	return step_;
    }
    void Append (float x) {
	chain_.Append(GetClientData(x));
    }
    void SetValues(double initial, double step, unsigned int nSteps) {
	initial_ = initial, step_ = step, nSteps_ = nSteps;
    }
    bool autoscale(void) {
	return autoscale_;
    }
    void SweepTicks(void) {
	if (autoscale_) {
	    if (ticks_ != NULL) {
		delete [] ticks_;
	    }
	    SetTicks();
	}
    }
    bool FirstTick(TickIter &iter) {
	ChainLink *linkPtr;

	linkPtr = chain_.FirstLink();
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

    const char *name_;		/* Name of the axis. Malloc-ed */

    unsigned int flags_;

    const char *title_;		/* Title of the axis. */

    const char *units_;		/* Units of the axis. */

    double valueMin_, valueMax_; /* The limits of the data. */

    double reqMin_, reqMax_;	/* Requested axis bounds. Consult the
				 * axisPtr->flags field for
				 * Axis::CONFIG_MIN and Axis::CONFIG_MAX
				 * to see if the requested bound have
				 * been set.  They override the
				 * computed range of the axis
				 * (determined by auto-scaling). */

    double min_, max_;		/* Smallest and largest major tick values for
				 * the axis.  If loose, the tick values lie
				 * outside the range of data values.  If
				 * tight, they lie interior to the limits of
				 * the data. */

    double range_;		/* Range of values on axis (max_ - min_) */
    double scale_;		/* Scale factor for axis (1.0/_range) */

    double reqStep_;		/* If > 0.0, overrides the computed major tick
				 * interval.  Otherwise a stepsize is
				 * automatically calculated, based upon the
				 * range of elements mapped to the axis. The
				 * default value is 0.0. */

    Ticks major_, minor_;

    void LogScale(void);
    void LinearScale(void);
    bool InRange(double x);
    void MakeTicks(void);

public:
    Axis(const char *name);
    ~Axis(void) {
	if (name_ != NULL) {
	    free((void *)name_);
	}
	if (units_ != NULL) {
	    free((void *)units_);
	}
	if (title_ != NULL) {
	    free((void *)title_);
	}
    }

    void ResetRange(void);
    void FixRange(double min, double max);
    void SetScale(double min, double max);
    double scale(void) {
	return scale_;
    }
    double range(void) {
	return range_;
    }
    bool FirstMajor(TickIter &iter) {
	return major_.FirstTick(iter);
    }
    bool FirstMinor(TickIter &iter) {
	return minor_.FirstTick(iter);
    }
    void GetDataLimits(double &min, double &max) {
	min = valueMin_, max = valueMax_;
    }
    double Map(double x);
    double InvMap(double x);
    const char *name(void) {
	return name_;
    }
    void name(const char *name) {
	if (name_ != NULL) {
	    free((void *)name_);
	}
	name_ = strdup(name);
    }
    const char *units(void) {
	return units_;
    }
    void units(const char *units) {
	if (units_ != NULL) {
	    free((void *)units_);
	}
	units_ = strdup(units);
    }
    const char *title(void) {
	return title_;
    }
    void title(const char *title) {
	if (title_ != NULL) {
	    free((void *)title_);
	}
	title_ = strdup(title);
    }
    double min(void) {
	return min_;		
    }
    void min(double min) {
	reqMin_ = min;	
    }
    double max(void) {
	return max_;		
    }
    void max(double max) {
	reqMax_ = max;	
    }
    void SetLimits(double min, double max) {
	reqMin_ = min, reqMax_ = max;
    }
    void UnsetLimits() {
	min(_NaN), max(_NaN);
    }
    void SetDescendingOption(bool value) {
	if (value) {
	    flags_ |= DESCENDING;
	} else {
	    flags_ &= ~DESCENDING;
	}
    }
    void SetTightMinOption(bool value) {
	if (value) {
	    flags_ |= TIGHT_MIN;
	} else {
	    flags_ &= ~TIGHT_MIN;
	}
    }
    void SetTightMaxOption(bool value) {
	if (value) {
	    flags_ |= TIGHT_MAX;
	} else {
	    flags_ &= ~TIGHT_MAX;
	}
    }
    void SetLogScaleOption(bool value) {
	if (value) {
	    flags_ |= LOGSCALE;
	} else {
	    flags_ &= ~LOGSCALE;
	}
    }
    void SetMajorStepOption(double value) {
	reqStep_ = value;	// Setting to 0.0 resets step to "auto"
    }
    void SetNumMinorTicksOption(int n) {
	minor_.reqNumTicks = n;
    }
    void SetNumMajorTicksOption(int n) {
	major_.reqNumTicks = n;
    }
};

#endif
