#ifndef _AXIS_RANGE_H
#define _AXIS_RANGE_H

class AxisRange {
    double min_, max_;
public:
    AxisRange(void) {
	min(0.0), max(1.0);
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
};

#endif /*_AXIS_RANGE_H*/
