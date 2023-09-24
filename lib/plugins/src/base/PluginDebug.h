#pragma once

#include <stdint.h>
#include <stddef.h>
#include <Print.h>

enum PDebugLevel { NONE, INFO, DEBUG, TRACE };

class PDebugClass {
    public:
    PDebugClass() {}
    template<typename... Args> size_t printf(PDebugLevel level, const char * f, Args... args) {
        if(isLevel(level))
           return print->printf(f, args...);
        return 0;
    }   

    void setPrint(Print* p) {
        print = p;
    }
    void setLevel(PDebugLevel l) {
        level = l;
    }
    bool isLevel(PDebugLevel l) {
        return (l<=level);
    }
    private:
    PDebugLevel level = PDebugLevel::DEBUG;
    Print* print = nullptr;
};
extern PDebugClass PDebug;