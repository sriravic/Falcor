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
#include "ConstantBuffer.h"
#include "ProgramVersion.h"
#include "buffer.h"
#include "glm/glm.hpp"
#include "texture.h"
#include "API/ProgramReflection.h"
#include "API/Device.h"

namespace Falcor
{
    ConstantBuffer::ConstantBuffer(const ProgramReflection::BufferReflection::SharedConstPtr& pReflector, size_t size) :
        VariablesBuffer(pReflector, size, 1, Buffer::BindFlags::Constant, Buffer::CpuAccess::Write)
    {
    }

    ConstantBuffer::SharedPtr ConstantBuffer::create(const ProgramReflection::BufferReflection::SharedConstPtr& pReflector, size_t overrideSize)
    {
        size_t size = (overrideSize == 0) ? pReflector->getRequiredSize() : overrideSize;        
        SharedPtr pBuffer = SharedPtr(new ConstantBuffer(pReflector, size));
        return pBuffer;
    }

    ConstantBuffer::SharedPtr ConstantBuffer::create(Program::SharedPtr& pProgram, const std::string& name, size_t overrideSize)
    {
        auto& pProgReflector = pProgram->getActiveVersion()->getReflector();
        auto& pBufferReflector = pProgReflector->getBufferDesc(name, ProgramReflection::BufferReflection::Type::Constant);
        if (pBufferReflector)
        {
            return create(pBufferReflector, overrideSize);
        }
        else
        {
            logError("Can't find a constant buffer named \"" + name + "\" in the program");
        }
        return nullptr;
    }

    void ConstantBuffer::uploadToGPU(size_t offset, size_t size) const
    {
        VariablesBuffer::uploadToGPU(offset, size);
        mCBV = nullptr;
    }

    DescriptorHeap::Entry ConstantBuffer::getCBV() const
    {
        if (mCBV == nullptr)
        {
            DescriptorHeap* pHeap = gpDevice->getSrvDescriptorHeap().get();

            mCBV = pHeap->allocateEntry();
            D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
            viewDesc.BufferLocation = getGpuAddress();
            viewDesc.SizeInBytes = (uint32_t)getSize();
            gpDevice->getApiHandle()->CreateConstantBufferView(&viewDesc, mCBV->getCpuHandle());
        }

        return mCBV;
    }
}