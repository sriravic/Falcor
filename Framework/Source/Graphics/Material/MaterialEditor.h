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
#pragma once
#include "Utils/Gui.h"
#include "Graphics/Material/Material.h"

namespace Falcor
{
    class Material;
    class Gui;
    
    class MaterialEditor
    {
    public:
        using UniquePtr = std::unique_ptr<MaterialEditor>;
        using UniqueConstPtr = std::unique_ptr<const MaterialEditor>;

        static UniquePtr create(const Material::SharedPtr& pMaterial, bool useSrgb);

        ~MaterialEditor() {};

        void renderGui(Gui* pGui);

    private:
        MaterialEditor(const Material::SharedPtr& pMaterial, bool useSrgb);

        Material::SharedPtr mpMaterial = nullptr;
        Gui::UniquePtr mpGui = nullptr;
        bool mUseSrgb;

        static Gui::DropdownList kLayerTypeDropdown;
        static Gui::DropdownList kLayerBlendDropdown;
        static Gui::DropdownList kLayerNDFDropdown;

        void initLambertLayer() const;
        void initConductorLayer() const;
        void initDielectricLayer() const;
        void initEmissiveLayer() const;

        void saveMaterial();

        void saveMaterial(Gui* pGui);

        // Per Material
        void setName(Gui* pGui);
        void setId(Gui* pGui);
        void setDoubleSided(Gui* pGui);
        void setHeightModifiers(Gui* pGui);
        void setAlphaThreshold(Gui* pGui);

        void setNormalMap(Gui* pGui);
        void setAlphaMap(Gui* pGui);
        void setHeightMap(Gui* pGui);
        // setAOMap

        void addLayer(Gui* pGui);

        // Per Layer
        void setLayerType(Gui* pGui, uint32_t layerID);
        void setLayerNdf(Gui* pGui, uint32_t layerID);
        void setLayerBlend(Gui* pGui, uint32_t layerID);
        void setLayerAlbedo(Gui* pGui, uint32_t layerID);
        void setLayerRoughness(Gui* pGui, uint32_t layerID);
        void setLayerTexture(Gui* pGui, uint32_t layerID);

        void setConductorLayerParams(Gui* pGui, uint32_t layerID);
        void setDialectricLayerParams(Gui* pGui, uint32_t layerID);

        bool removeLayer(Gui* pGui, uint32_t layerID); // #TODO this should probably return true if removed to terminate rest of elements being rendered


        Texture::SharedPtr changeTexture(Gui* pGui, const std::string& label, const Texture::SharedPtr& pCurrentTexture);

        static Material* getMaterial(void* pUserData);
        static MaterialLayerValues* getActiveLayerData(void* pUserData);
    };
}