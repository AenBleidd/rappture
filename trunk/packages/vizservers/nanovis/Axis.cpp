/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <string.h>

#include "Axis.h"

inline bool DEFINED(double x) {
    return !isnan(x);
}

inline double EXP10(double x) {
    return pow(10.0, x);
}

inline int ROUND(double x) {
    return (int)round(x);
}

inline double UROUND(double x, double u) {
    return (ROUND((x)/(u)))*u;
}

inline double UCEIL(double x, double u) {
    return (ceil((x)/(u)))*u;
}

inline double UFLOOR(double x, double u) {
    return (floor((x)/(u)))*u;
}

/**
 * Reference: Paul Heckbert, "Nice Numbers for Graph Labels",
 *            Graphics Gems, pp 61-63.  
 *
 * Finds a "nice" number approximately equal to x.
 */
static double
niceNum(double x,
        int round)              /* If non-zero, round. Otherwise take ceiling
                                 * of value. */
{
    double expt;                /* Exponent of x */
    double frac;                /* Fractional part of x */
    double nice;                /* Nice, rounded fraction */

    expt = floor(log10(x));
    frac = x / EXP10(expt);     /* between 1 and 10 */
    if (round) {
        if (frac < 1.5) {
            nice = 1.0;
        } else if (frac < 3.0) {
            nice = 2.0;
        } else if (frac < 7.0) {
            nice = 5.0;
        } else {
            nice = 10.0;
        }
    } else {
        if (frac <= 1.0) {
            nice = 1.0;
        } else if (frac <= 2.0) {
            nice = 2.0;
        } else if (frac <= 5.0) {
            nice = 5.0;
        } else {
            nice = 10.0;
        }
    }
    return nice * EXP10(expt);
}

void 
Ticks::setTicks()
{
    _numTicks = 0;
    _ticks = new float[_nSteps];
    if (_step == 0.0) { 
        /* Hack: A zero step indicates to use log values. */
        unsigned int i;
        /* Precomputed log10 values [1..10] */
        static double logTable[] = {
            0.0, 
            0.301029995663981, 
            0.477121254719662, 
            0.602059991327962, 
            0.698970004336019, 
            0.778151250383644, 
            0.845098040014257,
            0.903089986991944, 
            0.954242509439325, 
            1.0
        };
        for (i = 0; i < _nSteps; i++) {
            _ticks[i] = logTable[i];
        }
    } else {
        double value;
        unsigned int i;
    
        value = _initial;        /* Start from smallest axis tick */
        for (i = 0; i < _nSteps; i++) {
            value = _initial + (_step * i);
            _ticks[i] = UROUND(value, _step);
        }
    }
    _numTicks = _nSteps;
}

Axis::Axis(const char *axisName) :
    _name(strdup(axisName)),
    _flags(AUTOSCALE),
    _title(NULL),
    _units(NULL),
    _valueMin(DBL_MAX),
    _valueMax(-DBL_MAX),
    _reqMin(NAN),
    _reqMax(NAN),
    _min(DBL_MAX),
    _max(-DBL_MAX),
    _range(0.0),
    _scale(0.0),
    _reqStep(0.0),
    _major(5),
    _minor(2)
{
}

/**
 * \brief Determines if a value lies within a given range.
 *
 * The value is normalized by the current axis range. If the normalized
 * value is between [0.0..1.0] then it's in range.  The value is compared
 * to 0 and 1., where 0.0 is the minimum and 1.0 is the maximum.
 * DBL_EPSILON is the smallest number that can be represented on the host
 * machine, such that (1.0 + epsilon) != 1.0.
 *
 * Please note, *max* can't equal *min*.
 *
 * \return If the value is within the interval [min..max], 1 is returned; 0
 * otherwise.
 */
bool
Axis::inRange(double x)
{
    if (_range < DBL_EPSILON) {
        return (fabs(_max - x) >= DBL_EPSILON);
    } else {
        x = (x - _min) * _scale;
        return ((x >= -DBL_EPSILON) && ((x - 1.0) < DBL_EPSILON));
    }
}

void
Axis::fixRange(double min, double max)
{
    if (min == DBL_MAX) {
        if (DEFINED(_reqMin)) {
            min = _reqMin;
        } else {
            min = (_flags & LOGSCALE) ? 0.001 : 0.0;
        }
    }
    if (max == -DBL_MAX) {
        if (DEFINED(_reqMax)) {
            max = _reqMax;
        } else {
            max = 1.0;
        }
    }
    if (min >= max) {
        /*
         * There is no range of data (i.e. min is not less than max), so
         * manufacture one.
         */
        if (min == 0.0) {
            min = 0.0, max = 1.0;
        } else {
            max = min + (fabs(min) * 0.1);
        }
    }

    /*   
     * The axis limits are either the current data range or overridden by the
     * values selected by the user with the -min or -max options.
     */
    _valueMin = (DEFINED(_reqMin)) ? _reqMin : min;
    _valueMax = (DEFINED(_reqMax)) ? _reqMax : max;
    if (_valueMax < _valueMin) {
        /*   
         * If the limits still don't make sense, it's because one limit
         * configuration option (-min or -max) was set and the other default
         * (based upon the data) is too small or large.  Remedy this by making
         * up a new min or max from the user-defined limit.
         */
        if (!DEFINED(_reqMin)) {
            _valueMin = _valueMax - (fabs(_valueMax) * 0.1);
        }
        if (!DEFINED(_reqMax)) {
            _valueMax = _valueMin + (fabs(_valueMax) * 0.1);
        }
    }
}

/**
 * \brief Determine the range and units of a log scaled axis.
 *
 * Unless the axis limits are specified, the axis is scaled
 * automatically, where the smallest and largest major ticks encompass
 * the range of actual data values.  When an axis limit is specified,
 * that value represents the smallest(min)/largest(max) value in the
 * displayed range of values.
 *
 * Both manual and automatic scaling are affected by the step used.  By
 * default, the step is the largest power of ten to divide the range in
 * more than one piece.
 *
 * Automatic scaling:
 * Find the smallest number of units which contain the range of values.
 * The minimum and maximum major tick values will be represent the
 * range of values for the axis. This greatest number of major ticks
 * possible is 10.
 *
 * Manual scaling:
 * Make the minimum and maximum data values the represent the range of
 * the values for the axis.  The minimum and maximum major ticks will be
 * inclusive of this range.  This provides the largest area for plotting
 * and the expected results when the axis min and max values have be set
 * by the user (.e.g zooming).  The maximum number of major ticks is 20.
 *
 * For log scale, there's the possibility that the minimum and
 * maximum data values are the same magnitude.  To represent the
 * points properly, at least one full decade should be shown.
 * However, if you zoom a log scale plot, the results should be
 * predictable. Therefore, in that case, show only minor ticks.
 * Lastly, there should be an appropriate way to handle numbers
 * <=0.
 * <pre>
 *          maxY
 *            |    units = magnitude (of least significant digit)
 *            |    high  = largest unit tick < max axis value
 *      high _|    low   = smallest unit tick > min axis value
 *            |
 *            |    range = high - low
 *            |    # ticks = greatest factor of range/units
 *           _|
 *        U   |
 *        n   |
 *        i   |
 *        t  _|
 *            |
 *            |
 *            |
 *       low _|
 *            |
 *            |_minX________________maxX__
 *            |   |       |      |       |
 *     minY  low                        high
 *           minY
 *
 *      numTicks = Number of ticks
 *      min = Minimum value of axis
 *      max = Maximum value of axis
 *      range = Range of values (max - min)
 *
 * </pre>
 *
 *      If the number of decades is greater than ten, it is assumed
 *      that the full set of log-style ticks can't be drawn properly.
 */
void
Axis::logScale()
{
    double range;
    double tickMin, tickMax;
    double majorStep, minorStep;
    int nMajor, nMinor;
    double min, max;

    nMajor = nMinor = 0;
    /* Suppress compiler warnings. */
    majorStep = minorStep = 0.0;
    tickMin = tickMax = NAN;
    min = _valueMin, max = _valueMax;
    if (min < max) {
        min = (min != 0.0) ? log10(fabs(min)) : 0.0;
        max = (max != 0.0) ? log10(fabs(max)) : 1.0;

        tickMin = floor(min);
        tickMax = ceil(max);
        range = tickMax - tickMin;
        
        if (range > 10) {
            /* There are too many decades to display a major tick at every
             * decade.  Instead, treat the axis as a linear scale.  */
            range = niceNum(range, 0);
            majorStep = niceNum(range / _major.reqNumTicks, 1);
            tickMin = UFLOOR(tickMin, majorStep);
            tickMax = UCEIL(tickMax, majorStep);
            nMajor = (int)((tickMax - tickMin) / majorStep) + 1;
            minorStep = EXP10(floor(log10(majorStep)));
            if (minorStep == majorStep) {
                nMinor = 4, minorStep = 0.2;
            } else {
                nMinor = ROUND(majorStep / minorStep) - 1;
            }
        } else {
            if (tickMin == tickMax) {
                tickMax++;
            }
            majorStep = 1.0;
            nMajor = (int)(tickMax - tickMin + 1); /* FIXME: Check this. */
            
            minorStep = 0.0;    /* This is a special hack to pass
                                 * information to the SetTicks
                                 * method. An interval of 0.0 indicates
                                 *   1) this is a minor sweep and 
                                 *   2) the axis is log scale.  
                                 */
            nMinor = 10;
        }
        if ((_flags & TIGHT_MIN) || (DEFINED(_reqMin))) {
            tickMin = min;
            nMajor++;
        }
        if ((_flags & TIGHT_MAX) || (DEFINED(_reqMax))) {
            tickMax = max;
        }
    }
    _major.setValues(floor(tickMin), majorStep, nMajor);
    _minor.setValues(minorStep, minorStep, nMinor);
    _min = tickMin;
    _max = tickMax;
    _range = _max - _min;
    _scale = 1.0 / _range;
}

/**
 * \brief Determine the units of a linear scaled axis.
 *
 * The axis limits are either the range of the data values mapped
 * to the axis (autoscaled), or the values specified by the -min
 * and -max options (manual).
 *
 * If autoscaled, the smallest and largest major ticks will
 * encompass the range of data values.  If the -loose option is
 * selected, the next outer ticks are choosen.  If tight, the
 * ticks are at or inside of the data limits are used.
 *
 * If manually set, the ticks are at or inside the data limits
 * are used.  This makes sense for zooming.  You want the
 * selected range to represent the next limit, not something a
 * bit bigger.
 *
 * Note: I added an "always" value to the -loose option to force
 * the manually selected axes to be loose. It's probably
 * not a good idea.
 *
 * <pre>
 *          maxY
 *            |    units = magnitude (of least significant digit)
 *            |    high  = largest unit tick < max axis value
 *      high _|    low   = smallest unit tick > min axis value
 *            |
 *            |    range = high - low
 *            |    # ticks = greatest factor of range/units
 *           _|
 *        U   |
 *        n   |
 *        i   |
 *        t  _|
 *            |
 *            |
 *            |
 *       low _|
 *            |
 *            |_minX________________maxX__
 *            |   |       |      |       |
 *     minY  low                        high
 *           minY
 *
 * 	numTicks = Number of ticks
 * 	min = Minimum value of axis
 * 	max = Maximum value of axis
 * 	range = Range of values (max - min)
 *
 * </pre>
 *
 * Side Effects:
 *	The axis tick information is set.  The actual tick values will
 *	be generated later.
 */
void
Axis::linearScale()
{
    double step;
    double tickMin, tickMax;
    unsigned int nTicks;

    nTicks = 0;
    step = 1.0;
    /* Suppress compiler warning. */
    tickMin = tickMax = 0.0;
    if (_valueMin < _valueMax) {
        double range;

        range = _valueMax - _valueMin;
        /* Calculate the major tick stepping. */
        if (_reqStep > 0.0) {
            /* An interval was designated by the user.  Keep scaling it until
             * it fits comfortably within the current range of the axis.  */
            step = _reqStep;
            while ((2 * step) >= range) {
                step *= 0.5;
            }
        } else {
            range = niceNum(range, 0);
            step = niceNum(range / _major.reqNumTicks, 1);
        }
        
        /* Find the outer tick values. Add 0.0 to prevent getting -0.0. */
        tickMin = floor(_valueMin / step) * step + 0.0;
        tickMax = ceil(_valueMax / step) * step + 0.0;
        
        nTicks = ROUND((tickMax - tickMin) / step) + 1;
    } 
    _major.setValues(tickMin, step, nTicks);

    /*
     * The limits of the axis are either the range of the data ("tight") or at
     * the next outer tick interval ("loose").  The looseness or tightness has
     * to do with how the axis fits the range of data values.  This option is
     * overridden when the user sets an axis limit (by either -min or -max
     * option).  The axis limit is always at the selected limit (otherwise we
     * assume that user would have picked a different number).
     */
    _min = ((_flags & TIGHT_MIN)||(DEFINED(_reqMin))) ? _valueMin : tickMin;
    _max = ((_flags & TIGHT_MAX)||(DEFINED(_reqMax))) ? _valueMax : tickMax;
    _range = _max - _min;
    _scale = 1.0 / _range;

    /* Now calculate the minor tick step and number. */

    if ((_minor.reqNumTicks > 0) && (_minor.autoscale())) {
        nTicks = _minor.reqNumTicks - 1;
        step = 1.0 / (nTicks + 1);
    } else {
        nTicks = 0;             /* No minor ticks. */
        step = 0.5;             /* Don't set the minor tick interval to
                                 * 0.0. It makes the GenerateTicks routine
                                 * create minor log-scale tick marks.  */
    }
    _minor.setValues(step, step, nTicks);
}


void
Axis::setScale(double min, double max)
{
    fixRange(min, max);
    if (_flags & LOGSCALE) {
        logScale();
    } else {
        linearScale();
    }
    _major.sweepTicks();
    _minor.sweepTicks();
    makeTicks();
}

void
Axis::makeTicks()
{
    _major.reset();
    _minor.reset();
    int i;
    for (i = 0; i < _major.numTicks(); i++) {
        double t1, t2;
        int j;
        
        t1 = _major.tick(i);
        /* Minor ticks */
        for (j = 0; j < _minor.numTicks(); j++) {
            t2 = t1 + (_major.step() * _minor.tick(j));
            if (!inRange(t2)) {
                continue;
            }
            if (t1 == t2) {
                continue;        // Don't add duplicate minor ticks.
            }
            _minor.append(t2);
        }
        if (!inRange(t1)) {
            continue;
        }
        _major.append(t1);
    }
}

double
Axis::map(double x)
{
    if ((_flags & LOGSCALE) && (x != 0.0)) {
        x = log10(fabs(x));
    }
    /* Map graph coordinate to normalized coordinates [0..1] */
    x = (x - _min) * _scale;
    if (_flags & DESCENDING) {
        x = 1.0 - x;
    }
    return x;
}

double
Axis::invMap(double x)
{
    if (_flags & DESCENDING) {
        x = 1.0 - x;
    }
    x = (x * _range) + _min;
    if (_flags & LOGSCALE) {
        x = EXP10(x);
    }
    return x;
}
