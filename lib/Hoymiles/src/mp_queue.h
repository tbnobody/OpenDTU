#pragma once

#ifdef __AVR__
    #include <etl/queue.h>

    namespace std {
        template<typename T> using queue = etl::queue<T, 1000>;
    }
#else
    #include <queue>
#endif