/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef AXIS_RANGE_H
#define AXIS_RANGE_H

#include <string.h>

class AxisRange {
    double min_, max_;
    char *units_;
public:
    AxisRange(void) {
	min(0.0);
	max(1.0);
    	units_ = NULL;
    }
    ~AxisRange(void) {
	if (units_ != NULL) {
	    delete [] units_;
	}
    }
    void SetRange(double min, double max) {
	min_ = min, max_ = max;
    }
    double min(void) {
	return min_;
    }
    void min(double min) {
	min_ = min;
    }
    double max(void) {
	return max_;
    }
    void max(double max) {
	max_ = max;
    }
    const char *units(void) {
	return units_;
    }
    void units(const char *units) {
	if (units_ != NULL) {
	    delete [] units_;
	}
	if (units == NULL) {
	    units_ = NULL;
	} else {
	    units_ = new char [strlen(units) + 1];
	    strcpy(units_, units);
	}
    }
};

#endif
