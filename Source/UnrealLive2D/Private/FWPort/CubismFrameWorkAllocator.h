#pragma once

#include <CubismFramework.hpp>
#include <ICubismAllocator.hpp>

class CubismFrameWorkAllocator : public Csm::ICubismAllocator
{
    void* Allocate(const Csm::csmSizeType size);

    void Deallocate(void* memory);

    void* AllocateAligned(const Csm::csmSizeType size, const Csm::csmUint32 alignment);

    void DeallocateAligned(void* alignedMemory);
};