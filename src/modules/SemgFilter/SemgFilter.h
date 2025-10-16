#ifndef SEMG_FILTER_MODULE
#define SEMG_FILTER_MODULE

#include <filters.h> // available in: https://github.com/MartinBloedorn/libFilter
#include "../NotchFilter/NotchFilter.h"

class SemgFilter
{
public:
    static const float low_cuttoff_frequency;
    static const float high_cuttoff_frequency;

    static const float sampling_time; //seconds

    static const IIR::ORDER order; // Butterworth - Oder (OD1 to OD4)
    static const IIR::TYPE filter_type_high_pass;

    SemgFilter() = delete;

    static Filter high_pass;
    static Filter low_pass;
    static NotchFilter notch_60hz;  // Power line interference removal

    static float filter(float value);
    static float filterWithNotch(float value);  // Filter + notch (recommended for streaming)
    static void updateSamplingRate(float sampling_time_ms, int low_cutoff, int high_cutoff, bool preserveState = false);
    static void resetState();
};

#endif