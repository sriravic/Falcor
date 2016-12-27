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
#include "Framework.h"
#include "API/LowLevel/ResourceAllocator.h"
#include "API/Buffer.h"
#include "API/D3D/D3D12/D3D12Resource.h"

namespace Falcor
{
    ID3D12ResourcePtr createBuffer(Buffer::State initState, size_t size, const D3D12_HEAP_PROPERTIES& heapProps, Buffer::BindFlags bindFlags);

    ResourceAllocator::~ResourceAllocator()
    {
        executeDeferredReleases();
    }

    ResourceAllocator::SharedPtr ResourceAllocator::create(size_t pageSize, GpuFence::SharedPtr pFence)
    {
        SharedPtr pAllocator = SharedPtr(new ResourceAllocator(pageSize, pFence));
        pAllocator->allocateNewPage();
        return pAllocator;
    }

    void ResourceAllocator::allocateNewPage()
    {
        if (mpActivePage)
        {
            mUsedPages[mCurrentPageId] = std::move(mpActivePage);
        }

        if (mAvailablePages.size())
        {
            mpActivePage = std::move(mAvailablePages.front());
            mAvailablePages.pop();
            mpActivePage->allocationsCount = 0;
            mpActivePage->currentOffset = 0;
        }
        else
        {
            mpActivePage = std::make_unique<PageData>();
            mpActivePage->pResourceHandle = createBuffer(Buffer::State::GenericRead, mPageSize, kUploadHeapProps, Buffer::BindFlags::None);
            mpActivePage->gpuAddress = mpActivePage->pResourceHandle->GetGPUVirtualAddress();
            D3D12_RANGE readRange = {};
            d3d_call(mpActivePage->pResourceHandle->Map(0, &readRange, (void**)&mpActivePage->pData));
        }

        mpActivePage->currentOffset = 0;
        mCurrentPageId++;
    }

    void allocateMegaPage(size_t size, ResourceAllocator::AllocationData& data)
    {
        data.pageID = ResourceAllocator::AllocationData::kMegaPageId;

        data.pResourceHandle = createBuffer(Buffer::State::GenericRead, size, kUploadHeapProps, Buffer::BindFlags::None);
        data.gpuAddress = data.pResourceHandle->GetGPUVirtualAddress();
        D3D12_RANGE readRange = {};
        d3d_call(data.pResourceHandle->Map(0, &readRange, (void**)&data.pData));
    }

    ResourceAllocator::AllocationData ResourceAllocator::allocate(size_t size, size_t alignment)
    {
        AllocationData data;
        if (size > mPageSize)
        {
            allocateMegaPage(size, data);
        }
        else
        {
            // Calculate the start
            size_t currentOffset = align_to(alignment, mpActivePage->currentOffset);
            if (currentOffset + size > mPageSize)
            {
                currentOffset = 0;
                allocateNewPage();
            }

            data.pageID = mCurrentPageId;
            data.gpuAddress = mpActivePage->gpuAddress + currentOffset;
            data.pData = mpActivePage->pData + currentOffset;
            data.pResourceHandle = mpActivePage->pResourceHandle;
            mpActivePage->currentOffset = currentOffset + size;
            mpActivePage->allocationsCount++;
        }

        data.fenceValue = mpFence->getCpuValue();
        return data;
    }

    void ResourceAllocator::release(AllocationData& data)
    {
        if(data.pResourceHandle)
        {
            mDeferredReleases.push(data);
        }
    }

    void ResourceAllocator::executeDeferredReleases()
    {
        uint64_t gpuVal = mpFence->getGpuValue();
        while (mDeferredReleases.size() && mDeferredReleases.top().fenceValue < gpuVal)
        {
            const AllocationData& data = mDeferredReleases.top();
            if (data.pageID == mCurrentPageId)
            {
                mpActivePage->allocationsCount--;
                if (mpActivePage->allocationsCount == 0)
                {
                    mpActivePage->currentOffset = 0;
                }
            }
            else
            {
                if(data.pageID != AllocationData::kMegaPageId)
                {
                    auto& pData = mUsedPages[data.pageID];
                    pData->allocationsCount--;
                    if (pData->allocationsCount == 0)
                    {
                        mAvailablePages.push(std::move(pData));
                        mUsedPages.erase(data.pageID);
                    }
                }
                // else it's a mega-page. Popping it will release the resource
            }
            mDeferredReleases.pop();
        }
    }
}
