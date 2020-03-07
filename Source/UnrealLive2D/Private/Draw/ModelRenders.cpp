#include "ModelRenders.h"
#include "Rendering/CubismRenderer.hpp"
#include "CubismSepRender.h"
#include "RHIDefinitions.h"
#include "RHIStaticStates.h"

#include "UnrealLive2D.h"
#include "Engine/Texture2D.h"
#include "PipelineStateCache.h"

using namespace Csm;

void FModelRenders::SetUpBlendMode(
    Csm::CubismModel* tp_Model, 
    const Csm::csmInt32 drawableIndex,
    FGraphicsPipelineStateInitializer& GraphicsPSOInit
)
{
    Rendering::CubismRenderer::CubismBlendMode ts_BlendMode = tp_Model->GetDrawableBlendMode(drawableIndex);
    switch (ts_BlendMode)
    {
    case Rendering::CubismRenderer::CubismBlendMode_Normal:
    {
        GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI();
        break;
    }
    case Rendering::CubismRenderer::CubismBlendMode_Additive:
    {
        GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI();
        break;
    }
    case Rendering::CubismRenderer::CubismBlendMode_Multiplicative:
    {
        GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_DestColor, BF_InverseSourceAlpha, BO_Add, BF_Zero, BF_One>::GetRHI();
        break;
    }
    }
}

void FModelRenders::FillVertexBuffer(
    Csm::CubismModel* tp_Model, 
    const Csm::csmInt32 drawableIndex, 
    FVertexBufferRHIRef ScratchVertexBufferRHI, 
    FCubismRenderState* tp_States, 
    FRHICommandListImmediate& RHICmdList
)
{
    const csmInt32 td_NumVertext = tp_Model->GetDrawableVertexCount(drawableIndex);
    UE_LOG(LogCubism, Verbose, TEXT("FillVertexBuffer: Vertext buffer info %d|%d >> (%u, %u)"), drawableIndex, td_NumVertext, ScratchVertexBufferRHI->GetSize(), ScratchVertexBufferRHI->GetUsage());
    check(td_NumVertext == tp_States->VertexCount[drawableIndex]);
    {

        const csmFloat32* uvarray = reinterpret_cast<csmFloat32*>(const_cast<Live2D::Cubism::Core::csmVector2*>(tp_Model->GetDrawableVertexUvs(drawableIndex)));
        const csmFloat32* varray = const_cast<csmFloat32*>(tp_Model->GetDrawableVertices(drawableIndex));

        check(varray);
        check(uvarray);

        void* DrawableData = RHICmdList.LockVertexBuffer(ScratchVertexBufferRHI, 0, td_NumVertext * sizeof(FCubismVertex), RLM_WriteOnly);
        FCubismVertex* RESTRICT DestSamples = (FCubismVertex*)DrawableData;

        for (int32 td_VertexIndex = 0; td_VertexIndex < td_NumVertext; ++td_VertexIndex)
        {
            DestSamples[td_VertexIndex].Position.X = varray[td_VertexIndex * 2];
            DestSamples[td_VertexIndex].Position.Y = varray[td_VertexIndex * 2 + 1];
            DestSamples[td_VertexIndex].UV.X = uvarray[td_VertexIndex * 2];
            DestSamples[td_VertexIndex].UV.Y = uvarray[td_VertexIndex * 2 + 1];
        }

        RHICmdList.UnlockVertexBuffer(ScratchVertexBufferRHI);
    }
}

class UTexture2D* FModelRenders::GetTexture(
    Csm::CubismModel* tp_Model,
    const Csm::csmInt32 drawableIndex,
    FCubismRenderState* tp_States
)
{
    const csmInt32 td_TextrueNo = tp_Model->GetDrawableTextureIndices(drawableIndex);
    UTexture2D* tp_Texture = nullptr;
    if (tp_States->Textures.IsValidIndex(td_TextrueNo))
    {
        tp_Texture = tp_States->Textures[td_TextrueNo];
    }

    if (!IsValid(tp_Texture))
    {
        UE_LOG(LogCubism, Warning, TEXT("DrawSeparateToRenderTarget_RenderThread: Texture %d invalid"), td_TextrueNo);
        return nullptr;
    }

    return tp_Texture;
}

FMatrix FModelRenders::ConvertCubismMatrix(Csm::CubismMatrix44& InCubismMartix)
{
    FMatrix ts_Mat;

    ts_Mat.M[0][0] = InCubismMartix.GetArray()[0];
    ts_Mat.M[0][1] = InCubismMartix.GetArray()[1];
    ts_Mat.M[0][2] = InCubismMartix.GetArray()[2];
    ts_Mat.M[0][3] = InCubismMartix.GetArray()[3];

    ts_Mat.M[1][0] = InCubismMartix.GetArray()[4];
    ts_Mat.M[1][1] = InCubismMartix.GetArray()[5];
    ts_Mat.M[1][2] = InCubismMartix.GetArray()[6];
    ts_Mat.M[1][3] = InCubismMartix.GetArray()[7];

    ts_Mat.M[2][0] = InCubismMartix.GetArray()[8];
    ts_Mat.M[2][1] = InCubismMartix.GetArray()[9];
    ts_Mat.M[2][2] = InCubismMartix.GetArray()[10];
    ts_Mat.M[2][3] = InCubismMartix.GetArray()[11];

    ts_Mat.M[3][0] = InCubismMartix.GetArray()[12];
    ts_Mat.M[3][1] = InCubismMartix.GetArray()[13];
    ts_Mat.M[3][2] = InCubismMartix.GetArray()[14];
    ts_Mat.M[3][3] = InCubismMartix.GetArray()[15];

    return ts_Mat;
}



void FCubismVertexBuffer::InitRHI() 
{
    // create a static vertex buffer
    FRHIResourceCreateInfo CreateInfo;
    VertexBufferRHI = RHICreateVertexBuffer(sizeof(FCubismVertex) * 4, BUF_Static, CreateInfo);
    void* VoidPtr = RHILockVertexBuffer(VertexBufferRHI, 0, sizeof(FCubismVertex) * 4, RLM_WriteOnly);
    static const FCubismVertex Vertices[4] =
    {
        FCubismVertex(-0.9,-0.9, 0, 0),
        FCubismVertex(-0.9,+0.9, 0, 1),
        FCubismVertex(+0.9,-0.9, 1, 0),
        FCubismVertex(+0.9,+0.9, 1, 1),
    };
    FMemory::Memcpy(VoidPtr, Vertices, sizeof(FCubismVertex) * 4);
    RHIUnlockVertexBuffer(VertexBufferRHI);
}

TGlobalResource<FCubismVertexBuffer> GCubismVertexScreenBuffer;

void FCubismVertexDeclaration::InitRHI()
{
    FVertexDeclarationElementList Elements;
    uint32 Stride = sizeof(FCubismVertex);
    Elements.Add(FVertexElement(0, STRUCT_OFFSET(FCubismVertex, Position), VET_Float2, 0, Stride));
    Elements.Add(FVertexElement(0, STRUCT_OFFSET(FCubismVertex, UV), VET_Float2, 1, Stride));
    VertexDeclarationRHI = PipelineStateCache::GetOrCreateVertexDeclaration(Elements);
}

void FCubismVertexDeclaration::ReleaseRHI()
{
    VertexDeclarationRHI.SafeRelease();
}

TGlobalResource<FCubismVertexDeclaration> GCubismVertexDeclaration;
