#ifndef _AXIS_RANGE_H
#define _AXIS_RANGE_H

class AxisRange {
    double _min, _max;
public:
    AxisRange(void) {
	SetRange(0.0, 1.0);
    }
    void SetRange(double min, double max) {
	_min = min, _max = max;
    }
    double Min(void) {
	return _min;
    }
    double Max(void) {
	return _max;
    }
};

#endif /*_AXIS_RANGE_H*/
