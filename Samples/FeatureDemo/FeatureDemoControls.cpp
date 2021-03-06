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
#include "FeatureDemo.h"
#include "API/D3D/FalcorD3D.h"

Gui::DropdownList kSampleCountList = {
    { 1, "1"},
    { 2, "2" },
    { 4, "4" },
    { 8, "8" },
};

void FeatureDemo::initControls()
{
    mControls.resize(ControlID::Count);
    mControls[ControlID::SuperSampling] = { false, "INTERPOLATION_MODE", "sample" };
    mControls[ControlID::DisableSpecAA] = { false, "_MS_DISABLE_ROUGHNESS_FILTERING" };
    mControls[ControlID::EnableShadows] = { true, "_ENABLE_SHADOWS" };
    mControls[ControlID::EnableReflections] = { true, "_ENABLE_REFLECTIONS" };

    for (uint32_t i = 0 ; i < ControlID::Count ; i++)
    {
        applyLightingProgramControl((ControlID)i);
    }
}

void FeatureDemo::applyLightingProgramControl(ControlID controlId)
{
    const ProgramControl control = mControls[controlId];
    if (control.enabled)
    {
        mLightingPass.pProgram->addDefine(control.define, control.value);
    }
    else
    {
        mLightingPass.pProgram->removeDefine(control.define);
    }
}

void FeatureDemo::onGuiRender()
{
    if (mpGui->addButton("Load Model"))
    {
        std::string filename;
        if (openFileDialog(Model::kSupportedFileFormatsStr, filename))
        {
            loadModel(filename);
        }
    }

    if (mpGui->addButton("Load Scene"))
    {
        std::string filename;
        if (openFileDialog(Scene::kFileFormatString, filename))
        {
            loadScene(filename);
        }
    }

    if (mpSceneRenderer)
    {
        if (mpGui->addDropdown("Sample Count", kSampleCountList, mSampleCount))
        {
            onResizeSwapChain();
        }

        if (mpGui->addCheckBox("Super Sampling", mControls[ControlID::SuperSampling].enabled))
        {
            applyLightingProgramControl(ControlID::SuperSampling);
        }

        bool saaEnabled = !mControls[ControlID::DisableSpecAA].enabled;
        if (mpGui->addCheckBox("Specular AA", saaEnabled))
        {
            mControls[ControlID::DisableSpecAA].enabled = !saaEnabled;
            applyLightingProgramControl(ControlID::DisableSpecAA);
        }

        if (mpGui->addCheckBox("Reflections", mControls[ControlID::EnableReflections].enabled))
        {
            applyLightingProgramControl(ControlID::EnableReflections);
        }

        const Scene* pScene = mpSceneRenderer->getScene();

        vec2 depthRange(pScene->getActiveCamera()->getNearPlane(), pScene->getActiveCamera()->getFarPlane());
        if (mpGui->addFloat2Var("Depth Range", depthRange, 0, FLT_MAX))
        {
            pScene->getActiveCamera()->setDepthRange(depthRange.x, depthRange.y);
        }

        for(uint32_t i = 0 ; i < pScene->getLightCount() ; i++)
        {
            Light* pLight = pScene->getLight(i).get();
            pLight->renderUI(mpGui.get(), pLight->getName().c_str());
        }
        if(mpGui->beginGroup("Shadows"))
        {
            if (mpGui->addCheckBox("Enable Shadows", mControls[ControlID::EnableShadows].enabled))
            {
                applyLightingProgramControl(ControlID::EnableShadows);
            }
            if(mControls[ControlID::EnableShadows].enabled)
            {
                mpGui->addCheckBox("Update Map", mShadowPass.updateShadowMap);
                mShadowPass.pCsm->renderUi(mpGui.get());
            }
            mpGui->endGroup();
        }

        mpToneMapper->renderUI(mpGui.get(), "Tone-Mapping");

        if (mpGui->beginGroup("SSAO"))
        {
            mpGui->addCheckBox("Enable SSAO", mControls[ControlID::EnableSSAO].enabled);
            if(mControls[ControlID::EnableSSAO].enabled)
            {
                mSSAO.pSSAO->renderGui(mpGui.get());
            }
            mpGui->endGroup();
        }
    }
}
