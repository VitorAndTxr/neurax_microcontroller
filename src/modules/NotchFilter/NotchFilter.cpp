#include "NotchFilter.h"

NotchFilter::NotchFilter(float notch_freq_hz, float bandwidth_hz, float sample_rate_hz)
    : notch_freq(notch_freq_hz),
      bandwidth(bandwidth_hz),
      sample_rate(sample_rate_hz),
      z1(0.0f),
      z2(0.0f)
{
    computeCoefficients();
}

void NotchFilter::computeCoefficients()
{
    // Notch filter design using bilinear transformation
    // Reference: "Digital Signal Processing" by Oppenheim & Schafer

    // Normalized frequency (0 to π)
    float w0 = 2.0f * PI * notch_freq / sample_rate;

    // Bandwidth parameter (quality factor Q = f0 / bandwidth)
    float Q = notch_freq / bandwidth;
    float alpha = sin(w0) / (2.0f * Q);

    // Analog prototype coefficients
    float cos_w0 = cos(w0);

    // Notch filter transfer function:
    // H(s) = (s^2 + w0^2) / (s^2 + (w0/Q)*s + w0^2)
    //
    // After bilinear transform: s = 2*fs*(z-1)/(z+1)
    // Results in:
    // H(z) = (b0 + b1*z^-1 + b2*z^-2) / (a0 + a1*z^-1 + a2*z^-2)

    float a0 = 1.0f + alpha;
    float a1_temp = -2.0f * cos_w0;
    float a2_temp = 1.0f - alpha;

    float b0_temp = 1.0f;
    float b1_temp = -2.0f * cos_w0;
    float b2_temp = 1.0f;

    // Normalize by a0
    b0 = b0_temp / a0;
    b1 = b1_temp / a0;
    b2 = b2_temp / a0;
    a1 = a1_temp / a0;
    a2 = a2_temp / a0;

    // Note: a0 becomes 1 after normalization (implicit in Direct Form II)
}

float NotchFilter::filterIn(float input)
{
    // Direct Form II Transposed implementation
    // Most efficient structure for floating-point processors
    //
    // Difference equation:
    // y[n] = b0*x[n] + z1[n-1]
    // z1[n] = b1*x[n] - a1*y[n] + z2[n-1]
    // z2[n] = b2*x[n] - a2*y[n]

    float output = b0 * input + z1;
    z1 = b1 * input - a1 * output + z2;
    z2 = b2 * input - a2 * output;

    return output;
}

void NotchFilter::flush()
{
    z1 = 0.0f;
    z2 = 0.0f;
}

void NotchFilter::setNotchFrequency(float notch_freq_hz, bool doFlush)
{
    notch_freq = notch_freq_hz;
    computeCoefficients();
    if (doFlush) {
        flush();
    }
}

void NotchFilter::setBandwidth(float bandwidth_hz, bool doFlush)
{
    bandwidth = bandwidth_hz;
    computeCoefficients();
    if (doFlush) {
        flush();
    }
}

void NotchFilter::setSampleRate(float sample_rate_hz, bool doFlush)
{
    sample_rate = sample_rate_hz;
    computeCoefficients();
    if (doFlush) {
        flush();
    }
}
