#ifndef NOTCH_FILTER_MODULE
#define NOTCH_FILTER_MODULE

#include <Arduino.h>

/**
 * @brief Second-order IIR Notch Filter for power line interference removal
 *
 * Implements a digital notch filter using Direct Form II Transposed structure:
 * H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)
 *
 * Design method: Bilinear transformation
 * Typical use: Remove 60 Hz (or 50 Hz) power line noise from biosignals
 */
class NotchFilter
{
private:
    // Filter coefficients (normalized: a0 = 1)
    float b0, b1, b2;  // Numerator coefficients
    float a1, a2;      // Denominator coefficients (a0 = 1 implicit)

    // Filter state variables (Direct Form II Transposed)
    float z1, z2;      // Delay line states

    // Filter parameters
    float notch_freq;  // Center frequency to reject (Hz)
    float bandwidth;   // 3dB bandwidth (Hz)
    float sample_rate; // Sampling rate (Hz)

    // Compute filter coefficients using bilinear transform
    void computeCoefficients();

public:
    /**
     * @brief Construct a new Notch Filter
     *
     * @param notch_freq_hz Center frequency to reject (typically 60 Hz for US/Brazil, 50 Hz for Europe)
     * @param bandwidth_hz  3dB bandwidth (typically 1-5 Hz for narrow notch, 10-20 Hz for wide)
     * @param sample_rate_hz Sampling rate in Hz
     */
    NotchFilter(float notch_freq_hz = 60.0f, float bandwidth_hz = 5.0f, float sample_rate_hz = 500.0f);

    /**
     * @brief Filter one input sample
     *
     * @param input Raw input value
     * @return float Filtered output value
     */
    float filterIn(float input);

    /**
     * @brief Reset filter state to zero (clears history)
     */
    void flush();

    /**
     * @brief Update notch frequency (e.g., switch between 50/60 Hz)
     *
     * @param notch_freq_hz New center frequency
     * @param doFlush If true, reset filter state
     */
    void setNotchFrequency(float notch_freq_hz, bool doFlush = true);

    /**
     * @brief Update bandwidth (controls notch width)
     *
     * @param bandwidth_hz New bandwidth in Hz
     * @param doFlush If true, reset filter state
     */
    void setBandwidth(float bandwidth_hz, bool doFlush = true);

    /**
     * @brief Update sampling rate (must be called if sampling rate changes)
     *
     * @param sample_rate_hz New sampling rate in Hz
     * @param doFlush If true, reset filter state
     */
    void setSampleRate(float sample_rate_hz, bool doFlush = true);

    /**
     * @brief Get current notch frequency
     * @return float Center frequency in Hz
     */
    float getNotchFrequency() const { return notch_freq; }

    /**
     * @brief Get current bandwidth
     * @return float Bandwidth in Hz
     */
    float getBandwidth() const { return bandwidth; }

    /**
     * @brief Get current sample rate
     * @return float Sample rate in Hz
     */
    float getSampleRate() const { return sample_rate; }
};

#endif // NOTCH_FILTER_MODULE
