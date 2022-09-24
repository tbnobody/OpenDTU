#pragma once

#ifdef __AVR__
    #include <etl/vector.h>

    namespace std
    {
        template<typename T> using vector = etl::vector<T, 1000>;
        using etl::move;
    }
#else
    #include <vector>
#endif