#pragma once

#ifdef __AVR__
    #include <etl/memory.h> // unique_ptr
    #include <ArxSmartPtr.h> // shared_ptr

    namespace std {
        using etl::unique_ptr;
    }
#else
    #include <memory>
#endif