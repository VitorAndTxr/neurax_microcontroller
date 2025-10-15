#include "SemgFilter.h"

const float SemgFilter::low_cuttoff_frequency = SEMG_FILTER_LOW_CUTOFF_FREQUENCY;

const float SemgFilter::high_cuttoff_frequency = SEMG_FILTER_HIGH_CUTOFF_FREQUENCY;

const float SemgFilter::sampling_time = 1000.0f / 215.0f;

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

// Notch filter for 60 Hz power line interference
// Default: 60 Hz center, 2 Hz bandwidth, 215 Hz sampling rate
NotchFilter SemgFilter::notch_60hz(60.0f, 2.0f, 215.0f);

float SemgFilter::filter(float value)
{
    value = SemgFilter::low_pass.filterIn(value);
    value = SemgFilter::high_pass.filterIn(value);
    value = value * 2;
    return value;
}

float SemgFilter::filterWithNotch(float value)
{
    // Apply bandpass filter first (10-40 Hz or 10-100 Hz)
    value = SemgFilter::low_pass.filterIn(value);
    value = SemgFilter::high_pass.filterIn(value);

    // Then apply notch filter to remove 60 Hz interference
    value = SemgFilter::notch_60hz.filterIn(value);

    value = value * 10;
    return value;
}

void SemgFilter::updateSamplingRate(float sampling_time_ms, int low_cutoff, int high_cutoff, bool preserveState)
{
    float sampling_time_sec = sampling_time_ms / 1000.0f;
    float sample_rate_hz = 1000.0f / sampling_time_ms;

    // Update bandpass filters
    SemgFilter::low_pass.setSamplingTime(sampling_time_sec, !preserveState);
    SemgFilter::low_pass.setCutoffFreqHZ(high_cutoff);

    SemgFilter::high_pass.setSamplingTime(sampling_time_sec, !preserveState);
    SemgFilter::high_pass.setCutoffFreqHZ(low_cutoff);

    // Update notch filter - ALWAYS flush to prevent instability
    SemgFilter::notch_60hz.setSampleRate(sample_rate_hz, true);  // Force flush
}

void SemgFilter::resetState()
{
    SemgFilter::low_pass.flush();
    SemgFilter::high_pass.flush();
    SemgFilter::notch_60hz.flush();
}
