/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#ifdef FALCOR_D3D12
#include "Framework.h"
#include "API/Buffer.h"
#include "API/Device.h"

namespace Falcor
{
    static D3D12_HEAP_PROPERTIES kUploadHeapProps =
    {
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        0,
        0,
    };

    // D3D12TODO - this in in texture
    static const D3D12_HEAP_PROPERTIES kDefaultHeapProps =
    {
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        0,
        0
    };

    static const D3D12_HEAP_PROPERTIES kReadbackHeapProps =
    {
        D3D12_HEAP_TYPE_READBACK,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        0,
        0
    };
    
    ID3D12ResourcePtr createBuffer(size_t size, const D3D12_HEAP_PROPERTIES& heapProps)
    {
        ID3D12Device* pDevice = gpDevice->getApiHandle();

        // Create the buffer
        D3D12_RESOURCE_DESC bufDesc = {};
        bufDesc.Alignment = 0;
        bufDesc.DepthOrArraySize = 1;
        bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        bufDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufDesc.Height = 1;
        bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufDesc.MipLevels = 1;
        bufDesc.SampleDesc.Count = 1;
        bufDesc.SampleDesc.Quality = 0;
        bufDesc.Width = size;

        D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON;
        switch (heapProps.Type)
        {
        case D3D12_HEAP_TYPE_UPLOAD:
            initState = D3D12_RESOURCE_STATE_GENERIC_READ;
            break;
        case D3D12_HEAP_TYPE_READBACK:
            initState = D3D12_RESOURCE_STATE_COPY_DEST;
            break;
        }
        ID3D12ResourcePtr pApiHandle;
        d3d_call(pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, initState, nullptr, IID_PPV_ARGS(&pApiHandle)));

        // Map and upload data if needed
        return pApiHandle;
    }

    Buffer::SharedPtr Buffer::create(size_t size, BindFlags usage, CpuAccess cpuAccess, const void* pInitData)
    {
        if (usage == BindFlags::Constant)
        {
            size = align_to(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, size);
        }

        Buffer::SharedPtr pBuffer = SharedPtr(new Buffer(size, usage, cpuAccess));

        if (cpuAccess == CpuAccess::Write)
        {
            pBuffer->mApiHandle = createBuffer(size, kUploadHeapProps);
            pBuffer->map(MapType::WriteDiscard);
        }
        else if (cpuAccess == CpuAccess::Read)
        {
            pBuffer->mApiHandle = createBuffer(size, kReadbackHeapProps);
            pBuffer->map(MapType::Read);
        }
        else
        {
            assert(cpuAccess == CpuAccess::None);
            pBuffer->mApiHandle = createBuffer(size, kDefaultHeapProps);
        }

        if (pInitData)
        {
            pBuffer->updateData(pInitData, 0, size);
        }

        return pBuffer;
    }

    Buffer::~Buffer() = default;

    void Buffer::copy(Buffer* pDst) const
    {
    }

    void Buffer::copy(Buffer* pDst, size_t srcOffset, size_t dstOffset, size_t count) const
    {
    }

    void Buffer::updateData(const void* pData, size_t offset, size_t size)
    {
        // Clamp the offset and size
        if (adjustSizeOffsetParams(size, offset) == false)
        {
            logWarning("Buffer::updateData() - size and offset are invalid. Nothing to update.");
            return;
        }

        if (mUpdateFlags == CpuAccess::Write)
        {
            assert(mpMappedData);
            uint8_t* pDst = (uint8_t*)mpMappedData + offset;
            memcpy(pDst, pData, size);
        }
        else
        {
            if ((mUpdateFlags == CpuAccess::Read) && mpMappedData)
            {
                logWarning("Updating buffer data while it is mapped for CPU read");
            }
            gpDevice->getCopyContext()->updateBuffer(this, pData, offset, size);
        }
    }

    void Buffer::readData(void* pData, size_t offset, size_t size) const
    {
        UNSUPPORTED_IN_D3D12("Buffer::ReadData(). If you really need this, create the resource with CPU read flag, and use Buffer::Map()");
    }

    uint64_t Buffer::getBindlessHandle()
    {
        UNSUPPORTED_IN_D3D12("D3D12 buffers don't have bindless handles.");
        return 0;
    }

    void* Buffer::map(MapType type)
    {
        if (mpMappedData == nullptr)
        {
            D3D12_RANGE readRange = {};
            if (type == MapType::Read)
            {
                if (mUpdateFlags != CpuAccess::Read)
                {
                    logError("Trying to map a buffer for read, but it wasn't created with the read access type");
                    return nullptr;
                }
                readRange = { 0, mSize };
            }
            else
            {
                assert(type == MapType::WriteDiscard);
                if (mUpdateFlags != CpuAccess::Write)
                {
                    logError("Trying to map a buffer for write, but it wasn't created with the write access type");
                    return nullptr;
                }
            }

            d3d_call(mApiHandle->Map(0, &readRange, &mpMappedData));
        }
        return mpMappedData;
    }

    void Buffer::unmap()
    {

    }

    uint64_t Buffer::makeResident(Buffer::GpuAccessFlags flags/* = Buffer::GpuAccessFlags::ReadOnly*/) const
    {
        UNSUPPORTED_IN_D3D12("Buffer::makeResident()");
        return 0;
    }

    void Buffer::evict() const
    {
        UNSUPPORTED_IN_D3D12("Buffer::evict()");
    }
}
#endif // #ifdef FALCOR_D3D12