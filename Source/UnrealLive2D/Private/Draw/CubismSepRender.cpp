#include "CubismSepRender.h"
#include "RHICommandList.h"
#include "TextureResource.h"
#include "ProfilingDebugging/RealtimeGPUProfiler.h"
#include "RenderingThread.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GlobalShader.h"
#include "Shader.h"
#include "RHIStaticStates.h"
#include "PipelineStateCache.h"
#include "RHIDefinitions.h"
#include "FWPort/RawModel.h"
#include "Templates/SharedPointer.h"
#include "RHI.h"
#include "ImageUtils.h"
#include "FWPort/ModelRender.h"
#include "Engine/Texture2D.h"

#include "UnrealLive2D.h"
#include "ModelRenders.h"
#include "SceneInterface.h"

static bool GDrawTextureForTest = false;
FAutoConsoleVariableRef CVarDrawTextureForTest(
    TEXT("cubism.db.texture"),
    GDrawTextureForTest,
    TEXT("Draw texture instead of model for debug"),
    ECVF_Default);

static bool GCubismNoMask = false;
FAutoConsoleVariableRef CVarCubismNoMask(
    TEXT("cubism.db.nomask"),
    GCubismNoMask,
    TEXT("Draw model with no mask"),
    ECVF_Default);

#define _DRAW_TEXTURE_ GDrawTextureForTest
#define _DRAW_MODEL_ (!_DRAW_TEXTURE_)

//////////////////////////////////////////////////////////////////////////
using Csm::csmFloat32;
using Csm::csmInt32;
using Csm::csmUint16;

struct FCompiledSeparateDraw
{
    Csm::CubismModel *InModel = nullptr;
    FCubismRenderState *InStates = nullptr;
};

//////////////////////////////////////////////////////////////////////////

static void DrawSeparateToRenderTarget_RenderThread(
    FRHICommandListImmediate &RHICmdList,
    const FCompiledSeparateDraw &CompiledSeparateDraw,
    const FName &TextureRenderTargetName,
    FTextureRenderTargetResource *OutTextureRenderTargetResource,
    ERHIFeatureLevel::Type FeatureLevel)
{

#if WANTS_DRAW_MESH_EVENTS
    SCOPED_DRAW_EVENTF(RHICmdList, SceneCapture, TEXT("DrawSeparateToRenderTarget %s"), TextureRenderTargetName);
#else
    SCOPED_DRAW_EVENT(RHICmdList, DrawSeparateToRenderTarget_RenderThread);
#endif

    //////////////////////////////////////////////////////////////////////////
    check(CompiledSeparateDraw.InModel);
    Csm::CubismModel *tp_Model = CompiledSeparateDraw.InModel;
    FCubismRenderState *tp_States = CompiledSeparateDraw.InStates;

    check(tp_States);
    check(IsInRenderingThread());

    /** Mask绘制-基础版 */
    const bool tb_IsUsingMask = tp_Model->IsUsingMasking();
    if (tb_IsUsingMask)
    {
        CubismClippingManager_UE *_clippingManager = tp_States->_ClippingManager.Get();
        _clippingManager->SetupClippingContext(*tp_Model, tp_States);

        /** Draw mask to single texture if in low precise mode */
        if (!tp_States->Get_UseHighPreciseMask())
        {
            FModelRenders::RenderMask_Full(tp_States, RHICmdList, _clippingManager, FeatureLevel, tp_Model);
        }
    }

    /** 渲染模型主体 */
    if (_DRAW_MODEL_)
    {
        const csmInt32 drawableCount = tp_Model->GetDrawableCount();
        const csmInt32 *renderOrder = tp_Model->GetDrawableRenderOrders();

        // インデックスを描画順でソート
        for (csmInt32 i = 0; i < drawableCount; ++i)
        {
            const csmInt32 order = renderOrder[i];
            tp_States->_sortedDrawableIndexList[order] = i;
        }

        //////////////////////////////////////////////////////////////////////////
        {
            FRHITexture* RenderTargetTexture = OutTextureRenderTargetResource->GetRenderTargetTexture();
            RHICmdList.Transition(MakeArrayView(FModelRenders::ConvertTransitionResource(FExclusiveDepthStencil::DepthWrite_StencilWrite, RenderTargetTexture)));

            FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::Clear_Store);
            RHICmdList.BeginRenderPass(RPInfo, TEXT("DrawClear"));
            RHICmdList.EndRenderPass();
        }

        //////////////////////////////////////////////////////////////////////////

        // 描画
        for (csmInt32 i = 0; i < drawableCount; ++i)
        {
            const csmInt32 drawableIndex = tp_States->_sortedDrawableIndexList[i];

            // Drawableが表示状態でなければ処理をパスする
            if (!tp_Model->GetDrawableDynamicFlagIsVisible(drawableIndex))
            {
                continue;
            }

            const csmInt32 indexCount = tp_Model->GetDrawableVertexIndexCount(drawableIndex);
            if (0 == indexCount)
            {
                continue;
            }

            if (!tp_States->IndexBuffers.Contains(drawableIndex))
            {
                continue;
            }

            if (!tp_States->VertexBuffers.Contains(drawableIndex))
            {
                UE_LOG(LogCubism, Error, TEXT("DrawSeparateToRenderTarget_RenderThread: Vertext buffer not inited."));
                continue;
            }

            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            /** Mask relate */
            CubismClippingContext *clipContext = (tp_States->_ClippingManager.IsValid())
                                                     ? (*tp_States->_ClippingManager->GetClippingContextListForDraw())[drawableIndex]
                                                     : NULL;
            const bool tb_IsMaskDraw = (nullptr != clipContext);
            if (tb_IsMaskDraw)
            {
#if !UE_BUILD_SHIPPING
                if (GCubismNoMask)
                {
                    continue;
                }
#endif

                /** High precise mask draws to mask buffer per drawable */
                if (tp_States->Get_UseHighPreciseMask())
                {
                    FModelRenders::RenderMask_Single(tp_States, RHICmdList, FeatureLevel, tp_Model, clipContext);
                }

                FModelRenders::DrawSepMask(OutTextureRenderTargetResource, RHICmdList, FeatureLevel, tp_Model, drawableIndex, tp_States, clipContext);
            }
            else
            {
                FModelRenders::DrawSepNormal(OutTextureRenderTargetResource, RHICmdList, FeatureLevel, tp_Model, drawableIndex, tp_States);
            }
        }
    }

    if (_DRAW_TEXTURE_)
    {
        FModelRenders::DrawTestTexture(OutTextureRenderTargetResource, RHICmdList, FeatureLevel, tp_States);
    }

    tp_States->bRenderOver = true;
}

void FCubismSepRender::DrawSeparateToRenderTarget(
    class UWorld *World,
    class UTextureRenderTarget2D *OutputRenderTarget) const
{
    check(IsInGameThread());

    if (!OutputRenderTarget)
    {
        UE_LOG(LogCubism, Error, TEXT("FCubismSepRender::DrawSeparateToRenderTarget: OutputRenderTarget not valid"));
        return;
    }

    if (!IsValid(World))
    {
        UE_LOG(LogCubism, Error, TEXT("FCubismSepRender::DrawSeparateToRenderTarget: World not valid"));
        return;
    }

    if (!_Model.IsValid())
    {
        UE_LOG(LogCubism, Error, TEXT("FCubismSepRender::DrawSeparateToRenderTarget: _Model not valid"));
        return;
    }

    Csm::CubismModel *tp_Model = _Model->GetModel();
    if (!tp_Model)
    {
        UE_LOG(LogCubism, Error, TEXT("FCubismSepRender::DrawSeparateToRenderTarget: GetModel not valid"));
        return;
    }

    const FName TextureRenderTargetName = OutputRenderTarget->GetFName();
    FTextureRenderTargetResource *TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();

    FCompiledSeparateDraw ts_CompiledSeparateDraw;
    ts_CompiledSeparateDraw.InModel = tp_Model;
    ts_CompiledSeparateDraw.InStates = &RenderStates;

    ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();

    ENQUEUE_RENDER_COMMAND(CaptureCommand)
    (
        [ts_CompiledSeparateDraw, TextureRenderTargetResource, TextureRenderTargetName, FeatureLevel](FRHICommandListImmediate &RHICmdList)
        {
            DrawSeparateToRenderTarget_RenderThread(
                RHICmdList,
                ts_CompiledSeparateDraw,
                TextureRenderTargetName,
                TextureRenderTargetResource,
                FeatureLevel);
        });
}

void FCubismSepRender::InitRender(TSharedPtr<class FRawModel> InModel, const FModelConfig &InModelConfig)
{
    if (!InModel.IsValid())
    {
        /** You should never reach here, something is wrong. */
        checkNoEntry();
        return;
    }

    _Model = InModel;

    Csm::CubismModel *tp_Model = _Model->GetModel();
    if (!tp_Model)
    {
        /** Model passing to render must be ready */
        checkNoEntry();
        return;
    }

    LoadTextures();

    csmInt32 td_DrawableCount = tp_Model->GetDrawableCount();
    RenderStates._sortedDrawableIndexList.Resize(td_DrawableCount, 0);

    ENQUEUE_RENDER_COMMAND(CubismInit)
    (
        [=, this](FRHICommandListImmediate &RHICmdList)
        {
            for (csmInt32 td_DrawIter = 0; td_DrawIter < td_DrawableCount; td_DrawIter++)
            {
                /** Vertex buffer */
                const csmInt32 vcount = tp_Model->GetDrawableVertexCount(td_DrawIter);
                if (0 != vcount)
                {
                    FRHIResourceCreateInfo CreateInfo_Vert(TEXT("CubismInit"));
                    FBufferRHIRef ScratchVertexBufferRHI = RHICmdList.CreateVertexBuffer(vcount * sizeof(FCubismVertex), BUF_Dynamic, ERHIAccess::WritableMask, CreateInfo_Vert);

                    RenderStates.VertexBuffers.Add(td_DrawIter, ScratchVertexBufferRHI);
                    RenderStates.VertexCount.Add(td_DrawIter, vcount);
                    UE_LOG(LogCubism, Log, TEXT("FCubismSepRender::InitRender: Vertext buffer info %d >> (%u, %u)"), td_DrawIter, ScratchVertexBufferRHI->GetSize(), ScratchVertexBufferRHI->GetUsage());
                }

                /** Index buffer are static */
                const csmInt32 indexCount = tp_Model->GetDrawableVertexIndexCount(td_DrawIter);
                if (indexCount != 0)
                {
                    const csmUint16 *indexArray = const_cast<csmUint16 *>(tp_Model->GetDrawableVertexIndices(td_DrawIter));

                    FRHIResourceCreateInfo CreateInfo_Indice(TEXT("InitRender"));
                    FBufferRHIRef IndexBufferRHI = RHICmdList.CreateIndexBuffer(sizeof(uint16), sizeof(uint16) * indexCount, BUF_Static, CreateInfo_Indice);
                    void *VoidPtr = RHICmdList.LockBuffer(IndexBufferRHI, 0, sizeof(uint16) * indexCount, RLM_WriteOnly);
                    FMemory::Memcpy(VoidPtr, indexArray, indexCount * sizeof(uint16));
                    RHICmdList.UnlockBuffer(IndexBufferRHI);

                    RenderStates.IndexBuffers.Add(td_DrawIter, IndexBufferRHI);
                }

                UE_LOG(LogCubism, Log, TEXT("FCubismSepRender::InitRender: [%d/%d] V:%d I:%d"), td_DrawIter, td_DrawableCount, vcount, indexCount);
            }



            //////////////////////////////////////////////////////////////////////////
            RenderStates._ClippingManager.Reset();
            RenderStates._ClippingManager = MakeShared<CubismClippingManager_UE>();

            RenderStates._ClippingManager->Initialize(
                *tp_Model,
                tp_Model->GetDrawableCount(),
                tp_Model->GetDrawableMasks(),
                tp_Model->GetDrawableMaskCounts());

            const csmInt32 bufferHeight = RenderStates._ClippingManager->GetClippingMaskBufferSize();

            ETextureCreateFlags Flags = ETextureCreateFlags(TexCreate_None | TexCreate_RenderTargetable | TexCreate_ShaderResource);
            const FRHITextureCreateDesc Desc =
                FRHITextureCreateDesc::Create2D(TEXT("InitRenderSec"))
                .SetExtent((int32)bufferHeight, (int32)bufferHeight)
                .SetFormat(EPixelFormat::PF_B8G8R8A8)
                .SetNumMips(1)
                .SetNumSamples(1)
                .SetFlags(Flags)
                .SetInitialState(ERHIAccess::Unknown)
                .SetGPUMask(FRHIGPUMask::All())
                .SetClearValue(FClearValueBinding(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));

            RenderStates.MaskBuffer = RHICreateTexture(Desc);
            // TransitionResource(FExclusiveDepthStencil DepthStencilMode, FRHITexture * DepthTexture)

            //////////////////////////////////////////////////////////////////////////
            RenderStates.ClearStates();
            RenderStates.RenderModelConfig = InModelConfig;
        });





    UE_LOG(LogCubism, Log, TEXT("FCubismSepRender::InitRender: Completed"));
}

void FCubismSepRender::LoadTextures()
{
    check(_Model.IsValid());

    Csm::ICubismModelSetting *_modelSetting = _Model->Get_ModelSetting();
    csmInt32 td_TextureNum = _modelSetting->GetTextureCount();

    /** Clear old textures */
    UnLoadTextures();
    RenderStates.Textures.SetNumZeroed(td_TextureNum);

    /** Load new texture */
    for (csmInt32 modelTextureNumber = 0; modelTextureNumber < td_TextureNum; modelTextureNumber++)
    {
        if (strcmp(_modelSetting->GetTextureFileName(modelTextureNumber), "") == 0)
        {
            continue;
        }

        Csm::csmString texturePath = _modelSetting->GetTextureFileName(modelTextureNumber);
        FString tstr_TempReadPath = _Model->Get_HomeDir() / UTF8_TO_TCHAR(texturePath.GetRawString());

        UTexture2D *tp_LoadedImage = FImageUtils::ImportFileAsTexture2D(tstr_TempReadPath);
        if (IsValid(tp_LoadedImage))
        {
            RenderStates.Textures[modelTextureNumber] = tp_LoadedImage;
        }

        /** Some kind of cheat, must call RemoveFromRoot as UnLoadTextures does if you directly remove some texture  */
        tp_LoadedImage->AddToRoot();
        UE_LOG(LogCubism, Log, TEXT("FCubismSepRender::LoadTextures: From %s | %d / %d >> %p"), *tstr_TempReadPath, modelTextureNumber, td_TextureNum, tp_LoadedImage);
    }

    UE_LOG(LogCubism, Log, TEXT("FCubismSepRender::LoadTextures: Complete with %d texture"), _modelSetting->GetTextureCount());
}

void FCubismSepRender::UnLoadTextures()
{
    for (UTexture2D *tp_Texture : RenderStates.Textures)
    {
        if (IsValid(tp_Texture))
        {
            tp_Texture->RemoveFromRoot();
        }
    }

    RenderStates.Textures.Empty();
}

//////////////////////////////////////////////////////////////////////////
void FCubismRenderState::ClearStates()
{
    bNoLowPreciseMask = false;
}

bool FCubismRenderState::Get_UseHighPreciseMask() const
{
    /** Config decide not use low precise mask */
    if (!RenderModelConfig.bTryLowPreciseMask)
    {
        return true;
    }

    /**
     * Low precise is not possible
     * @Note See SetupClippingContext for detail
     */
    if (bNoLowPreciseMask)
    {
        return true;
    }

    return false;
}
