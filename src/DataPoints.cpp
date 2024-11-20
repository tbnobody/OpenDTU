#include <stdio.h>

#include "DataPoints.h"

static char conversionBuffer[16];

template<typename T>
std::string dataPointValueToStr(T const& v) {
    snprintf(conversionBuffer, sizeof(conversionBuffer), "%d", v);
    return conversionBuffer;
}

// explicit instanciations for the above unspecialized implementation
template std::string dataPointValueToStr(int16_t const& v);
template std::string dataPointValueToStr(int32_t const& v);
template std::string dataPointValueToStr(uint8_t const& v);
template std::string dataPointValueToStr(uint16_t const& v);
template std::string dataPointValueToStr(uint32_t const& v);

template<>
std::string dataPointValueToStr(std::string const& v) {
    return v;
}

template<>
std::string dataPointValueToStr(bool const& v) {
    return v?"yes":"no";
}

template<>
std::string dataPointValueToStr(tCellVoltages const& v) {
    std::string res;
    res.reserve(v.size()*(2+2+1+4)); // separator, index, equal sign, value
    res += "(";
    std::string sep = "";
    for(auto const& mapval : v) {
        snprintf(conversionBuffer, sizeof(conversionBuffer), "%s%d=%d",
                sep.c_str(), mapval.first, mapval.second);
        res += conversionBuffer;
        sep = ", ";
    }
    res += ")";
    return std::move(res);
}
