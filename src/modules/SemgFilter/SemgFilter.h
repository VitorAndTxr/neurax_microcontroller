#ifndef SEMG_FILTER_MODULE
#define SEMG_FILTER_MODULE

#include <filters.h> // available in: https://github.com/MartinBloedorn/libFilter

class SemgFilter
{
public:
    static const float low_cuttoff_frequency;  
    static const float high_cuttoff_frequency;  

    static const float sampling_time; //seconds

    static const IIR::ORDER order; // Butterworth - Oder (OD1 to OD4)
    static const IIR::TYPE filter_type_high_pass;

    SemgFilter() = delete;

    static const Filter high_pass;
    static const Filter low_pass;
};

#endif