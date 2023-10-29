#ifndef SEMG_FILTER_MODULE
#define SEMG_FILTER_MODULE

class SemgFilter
{
private:
    static const float f1 = SEMG_FILTER_LOW_CUTOFF_FREQUENCY;  //Cutoff frequency in Hz
    static const float f2 = SEMG_FILTER_HIGH_CUTOFF_FREQUENCY;  //Cutoff frequency in Hz

    static const float sampling_time = SEMG_SAMPLING_TIME; //seconds.

    static const IIR::ORDER order = IIR::ORDER::OD3; // Butterworth - Oder (OD1 to OD4)
    static const IIR::TYPE typeHP = IIR::TYPE::HIGHPASS;

public:
    SemgFilter(/* args */);
    ~SemgFilter();

    Filter* high_pass;
    Filter* low_pass;
};

#endif