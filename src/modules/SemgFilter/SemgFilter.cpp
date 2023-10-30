#include "SemgFilter.h"

const float SemgFilter::low_cuttoff_frequency = SEMG_FILTER_LOW_CUTOFF_FREQUENCY;

const float SemgFilter::high_cuttoff_frequency = SEMG_FILTER_HIGH_CUTOFF_FREQUENCY;

const float SemgFilter::sampling_time = SEMG_SAMPLING_TIME; 

const IIR::ORDER SemgFilter::order = IIR::ORDER::OD3; // Butterworth - Oder (OD1 to OD4)
const IIR::TYPE SemgFilter::filter_type_high_pass = IIR::TYPE::HIGHPASS;

Filter SemgFilter::high_pass(
    SemgFilter::low_cuttoff_frequency, 
    SemgFilter::sampling_time, 
    SemgFilter::order, 
    SemgFilter::filter_type_high_pass
);

Filter SemgFilter::low_pass(
    SemgFilter::high_cuttoff_frequency, 
    SemgFilter::sampling_time, 
    SemgFilter::order
);

float SemgFilter::filter(float value)
{
    value = SemgFilter::low_pass.filterIn(value);
    value = SemgFilter::high_pass.filterIn(value);
    value = abs(value) * 100;
    return value;
}
