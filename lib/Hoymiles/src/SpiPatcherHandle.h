#pragma once

class SpiPatcherHandle
{
public:
    SpiPatcherHandle();
    virtual ~SpiPatcherHandle();

    virtual void patch() = 0;
    virtual void unpatch() = 0;
};
