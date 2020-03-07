#include "ModelRenders.h"
#include "FWPort/ModelRender.h"
#include "Model/CubismModel.hpp"
#include "CubismSepRender.h"
#include "GlobalShader.h"
#include "RHIStaticStates.h"
#include "RHIResources.h"

#include "UnrealLive2D.h"
#include "PipelineStateCache.h"
#include "Engine/Texture2D.h"
#include "ShaderParameterUtils.h"


//////////////////////////////////////////////////////////////////////////
class FCubismMaskShader : public FGlobalShader
{
    DECLARE_INLINE_TYPE_LAYOUT(FCubismMaskShader, NonVirtual);;

public:
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }

    static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    {
        FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
    }

    FCubismMaskShader() {}

    FCubismMaskShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer)
    {
        TestFloat.Bind(Initializer.ParameterMap, TEXT("TestFloat"));
        ProjectMatrix.Bind(Initializer.ParameterMap, TEXT("ProjectMatrix"));
        BaseColor.Bind(Initializer.ParameterMap, TEXT("BaseColor"));
        ChannelFlag.Bind(Initializer.ParameterMap, TEXT("ChannelFlag"));

        MainTexture.Bind(Initializer.ParameterMap, TEXT("MainTexture"));
        MainTextureSampler.Bind(Initializer.ParameterMap, TEXT("MainTextureSampler"));
    }

    template<typename TShaderRHIParamRef>
    void SetParameters(
        FRHICommandListImmediate& RHICmdList,
        const TShaderRHIParamRef ShaderRHI,
        const FMatrix& InProjectMatrix,
        const FVector4& InBaseColor,
        const FVector4& InChannelFlag,
        FTextureRHIRef ShaderResourceTexture
    )
    {
        SetShaderValue(RHICmdList, ShaderRHI, TestFloat, 1.0f);
        SetShaderValue(RHICmdList, ShaderRHI, ProjectMatrix, InProjectMatrix);
        SetShaderValue(RHICmdList, ShaderRHI, BaseColor, InBaseColor);
        SetShaderValue(RHICmdList, ShaderRHI, ChannelFlag, InChannelFlag);

        SetTextureParameter(RHICmdList, ShaderRHI, MainTexture, ShaderResourceTexture);
        SetSamplerParameter(RHICmdList, ShaderRHI, MainTextureSampler, TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI());
    }

private:
    LAYOUT_FIELD(FShaderParameter, TestFloat);
    LAYOUT_FIELD(FShaderParameter, ProjectMatrix);
    LAYOUT_FIELD(FShaderParameter, BaseColor);
    LAYOUT_FIELD(FShaderParameter, ChannelFlag);
    
    LAYOUT_FIELD(FShaderResourceParameter, MainTexture);
    LAYOUT_FIELD(FShaderResourceParameter, MainTextureSampler);
};

class FCubismMaskVS : public FCubismMaskShader
{
    DECLARE_SHADER_TYPE(FCubismMaskVS, Global);

public:

    /** Default constructor. */
    FCubismMaskVS() {}

    /** Initialization constructor. */
    FCubismMaskVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FCubismMaskShader(Initializer)
    {
    }

private:
};

class FCubismMaskPS : public FCubismMaskShader
{
    DECLARE_SHADER_TYPE(FCubismMaskPS, Global);

public:

    /** Default constructor. */
    FCubismMaskPS() {}

    /** Initialization constructor. */
    FCubismMaskPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FCubismMaskShader(Initializer)
    {

    }
};

IMPLEMENT_SHADER_TYPE(, FCubismMaskVS, TEXT("/Plugin/UnrealLive2D/Private/CubismMask.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FCubismMaskPS, TEXT("/Plugin/UnrealLive2D/Private/CubismMask.usf"), TEXT("MainPS"), SF_Pixel)

//////////////////////////////////////////////////////////////////////////


void FillMaskParameter(CubismClippingContext* clipContext, CubismClippingManager_UE* _clippingManager, FMatrix& ts_MartixForMask, FVector4& ts_BaseColor, FVector4& ts_ChanelFlag)
{
    // チャンネル
    const csmInt32 channelNo = clipContext->_layoutChannelNo;
    // チャンネルをRGBAに変換
    Csm::Rendering::CubismRenderer::CubismTextureColor* colorChannel = _clippingManager->GetChannelFlagAsColor(channelNo);

    csmRectF* rect = clipContext->_layoutBounds;

    ts_MartixForMask = FModelRenders::ConvertCubismMatrix(clipContext->_matrixForMask);
    ts_BaseColor = FVector4(rect->X * 2.0f - 1.0f, rect->Y * 2.0f - 1.0f, rect->GetRight() * 2.0f - 1.0f, rect->GetBottom() * 2.0f - 1.0f);
    ts_ChanelFlag = FVector4(colorChannel->R, colorChannel->G, colorChannel->B, colorChannel->A);
}

void FModelRenders::RenderMask_Full(
    FCubismRenderState* tp_States, 
    FRHICommandListImmediate& RHICmdList, 
    CubismClippingManager_UE* _clippingManager, 
    ERHIFeatureLevel::Type FeatureLevel, 
    Csm::CubismModel* tp_Model
)
{
    //////////////////////////////////////////////////////////////////////////
    FRHITexture2D* RenderTargetTexture = tp_States->MaskBuffer;

    RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, RenderTargetTexture);

    FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::Clear_Store, RenderTargetTexture);
    RHICmdList.BeginRenderPass(RPInfo, TEXT("DrawMask01"));
    {

        // Update viewport.
        float tf_MaskSize = _clippingManager->_clippingMaskBufferSize;
        RHICmdList.SetViewport(
            0, 0, 0.f,
            tf_MaskSize, tf_MaskSize, 1.f);

        // Get shaders.
        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
        TShaderMapRef< FCubismMaskVS > VertexShader(GlobalShaderMap);
        TShaderMapRef< FCubismMaskPS > PixelShader(GlobalShaderMap);

        //PixelShader->SetMaskParameter(RHICmdList, ts_Temp, FVector4(1, 1, 1, 1), FVector4(1, 1, 1, 1), nullptr);

        // Set the graphic pipeline state.
        FGraphicsPipelineStateInitializer GraphicsPSOInit;
        RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
        GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
        GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_Zero, BF_InverseSourceColor, BO_Add, BF_Zero, BF_InverseSourceAlpha>::GetRHI();
        GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
        GraphicsPSOInit.PrimitiveType = PT_TriangleList;
        GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GCubismVertexDeclaration.VertexDeclarationRHI;
        GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
        GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

        //////////////////////////////////////////////////////////////////////////
        for (csmUint32 clipIndex = 0; clipIndex < _clippingManager->_clippingContextListForMask.GetSize(); clipIndex++)
        {
            CubismClippingContext* clipContext = _clippingManager->_clippingContextListForMask[clipIndex];

            const csmInt32 clipDrawCount = clipContext->_clippingIdCount;
            for (csmInt32 i = 0; i < clipDrawCount; i++)
            {
                const csmInt32 clipDrawIndex = clipContext->_clippingIdList[i];

                // 頂点情報が更新されておらず、信頼性がない場合は描画をパスする
                if (!tp_Model->GetDrawableDynamicFlagVertexPositionsDidChange(clipDrawIndex))
                {
                    continue;
                }

                // TODO: push [IsCulling] to [RasterizerState] if needed
                const bool tb_IsCulling = (tp_Model->GetDrawableCulling(clipDrawIndex) != 0);

                const csmInt32 td_TextrueNo = tp_Model->GetDrawableTextureIndices(clipDrawIndex);
                UTexture2D* tp_Texture = nullptr;
                if (tp_States->Textures.IsValidIndex(td_TextrueNo))
                {
                    tp_Texture = tp_States->Textures[td_TextrueNo];
                }

                if (!IsValid(tp_Texture))
                {
                    UE_LOG(LogCubism, Warning, TEXT("DrawSeparateToRenderTarget_RenderThread:[Mask] Texture %d invalid"), td_TextrueNo);
                    continue;
                }

                const csmInt32 indexCount = tp_Model->GetDrawableVertexIndexCount(clipDrawIndex);
                if (0 == indexCount)
                {
                    continue;
                }

                if (!tp_States->IndexBuffers.Contains(clipDrawIndex))
                {
                    continue;
                }

                if (!tp_States->VertexBuffers.Contains(clipDrawIndex))
                {
                    UE_LOG(LogCubism, Error, TEXT("DrawSeparateToRenderTarget_RenderThread:[Mask] Vertext buffer not inited."));
                    continue;
                }

                //////////////////////////////////////////////////////////////////////////
                csmFloat32 tf_Opacity = tp_Model->GetDrawableOpacity(clipDrawIndex);
                Rendering::CubismRenderer::CubismBlendMode ts_BlendMode = tp_Model->GetDrawableBlendMode(clipDrawIndex);
                csmBool tb_InvertMask = tp_Model->GetDrawableInvertedMask(clipDrawIndex);



                //////////////////////////////////////////////////////////////////////////
                /** Drawable draw */
                const csmInt32 td_NumVertext = tp_Model->GetDrawableVertexCount(clipDrawIndex);

                FIndexBufferRHIRef IndexBufferRHI = tp_States->IndexBuffers.FindRef(clipDrawIndex);
                FVertexBufferRHIRef ScratchVertexBufferRHI = tp_States->VertexBuffers.FindRef(clipDrawIndex);
                FTextureRHIRef tsr_TextureRHI = tp_Texture->Resource->TextureRHI;

                FillVertexBuffer(tp_Model, clipDrawIndex, ScratchVertexBufferRHI, tp_States, RHICmdList);

                ////////////////////////////////////////////////////////////////////////////

                FMatrix ts_MartixForMask;
                FVector4 ts_BaseColor;
                FVector4 ts_ChanelFlag;

                FillMaskParameter(clipContext, _clippingManager, ts_MartixForMask, ts_BaseColor, ts_ChanelFlag);

                VertexShader->SetParameters(RHICmdList, VertexShader.GetVertexShader(), ts_MartixForMask, ts_BaseColor, ts_ChanelFlag, tsr_TextureRHI);
                PixelShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), ts_MartixForMask, ts_BaseColor, ts_ChanelFlag, tsr_TextureRHI);
                SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

                ////////////////////////////////////////////////////////////////////////////

                RHICmdList.SetStreamSource(0, ScratchVertexBufferRHI, 0);

                RHICmdList.DrawIndexedPrimitive(
                    IndexBufferRHI,
                    /*BaseVertexIndex=*/ 0,
                    /*MinIndex=*/ 0,
                    /*NumVertices=*/ td_NumVertext,
                    /*StartIndex=*/ 0,
                    /*NumPrimitives=*/ indexCount / 3,
                    /*NumInstances=*/ 1
                );



            }
        }

        //FMatrix ts_Temp = FMatrix::Identity;
        //VertexShader->SetParameters(RHICmdList, VertexShader->GetVertexShader(), ts_Temp);
        //PixelShader->SetParameters(RHICmdList, PixelShader->GetPixelShader(), ts_Temp);
        //SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

        //RHICmdList.SetStreamSource(0, GCubismVertexScreenBuffer.VertexBufferRHI, 0);
        //RHICmdList.DrawIndexedPrimitive(GTwoTrianglesIndexBuffer.IndexBufferRHI, 0, 0, 4, 0, 2, 1);

    }
    RHICmdList.EndRenderPass();
}

void FModelRenders::RenderMask_Single(
    FCubismRenderState* tp_States, 
    FRHICommandListImmediate& RHICmdList, 
    ERHIFeatureLevel::Type FeatureLevel, 
    Csm::CubismModel* tp_Model, 
    class CubismClippingContext* clipContext
)
{
    CubismClippingManager_UE* _clippingManager = tp_States->_ClippingManager.Get();
    check(_clippingManager);

    //////////////////////////////////////////////////////////////////////////
    FRHITexture2D* RenderTargetTexture = tp_States->MaskBuffer;

    RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, RenderTargetTexture);

    FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::Clear_Store, RenderTargetTexture);
    RHICmdList.BeginRenderPass(RPInfo, TEXT("DrawMask02"));
    {

        // Update viewport.
        float tf_MaskSize = _clippingManager->_clippingMaskBufferSize;
        RHICmdList.SetViewport(
            0, 0, 0.f,
            tf_MaskSize, tf_MaskSize, 1.f);

        // Get shaders.
        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
        TShaderMapRef< FCubismMaskVS > VertexShader(GlobalShaderMap);
        TShaderMapRef< FCubismMaskPS > PixelShader(GlobalShaderMap);

        //PixelShader->SetMaskParameter(RHICmdList, ts_Temp, FVector4(1, 1, 1, 1), FVector4(1, 1, 1, 1), nullptr);

        // Set the graphic pipeline state.
        FGraphicsPipelineStateInitializer GraphicsPSOInit;
        RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
        GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
        GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_Zero, BF_InverseSourceColor, BO_Add, BF_Zero, BF_InverseSourceAlpha>::GetRHI();
        GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
        GraphicsPSOInit.PrimitiveType = PT_TriangleList;
        GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GCubismVertexDeclaration.VertexDeclarationRHI;
        GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
        GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

        //////////////////////////////////////////////////////////////////////////

        const csmInt32 clipDrawCount = clipContext->_clippingIdCount;
        for (csmInt32 i = 0; i < clipDrawCount; i++)
        {
            const csmInt32 clipDrawIndex = clipContext->_clippingIdList[i];

            // 頂点情報が更新されておらず、信頼性がない場合は描画をパスする
            if (!tp_Model->GetDrawableDynamicFlagVertexPositionsDidChange(clipDrawIndex))
            {
                continue;
            }

            // TODO: push [IsCulling] to [RasterizerState] if needed
            const bool tb_IsCulling = (tp_Model->GetDrawableCulling(clipDrawIndex) != 0);

            const csmInt32 td_TextrueNo = tp_Model->GetDrawableTextureIndices(clipDrawIndex);
            UTexture2D* tp_Texture = nullptr;
            if (tp_States->Textures.IsValidIndex(td_TextrueNo))
            {
                tp_Texture = tp_States->Textures[td_TextrueNo];
            }

            if (!IsValid(tp_Texture))
            {
                UE_LOG(LogCubism, Warning, TEXT("DrawSeparateToRenderTarget_RenderThread:[Mask] Texture %d invalid"), td_TextrueNo);
                continue;
            }

            const csmInt32 indexCount = tp_Model->GetDrawableVertexIndexCount(clipDrawIndex);
            if (0 == indexCount)
            {
                continue;
            }

            if (!tp_States->IndexBuffers.Contains(clipDrawIndex))
            {
                continue;
            }

            if (!tp_States->VertexBuffers.Contains(clipDrawIndex))
            {
                UE_LOG(LogCubism, Error, TEXT("DrawSeparateToRenderTarget_RenderThread:[Mask] Vertext buffer not inited."));
                continue;
            }

            //////////////////////////////////////////////////////////////////////////
            /** Drawable draw */
            const csmInt32 td_NumVertext = tp_Model->GetDrawableVertexCount(clipDrawIndex);

            FIndexBufferRHIRef IndexBufferRHI = tp_States->IndexBuffers.FindRef(clipDrawIndex);
            FVertexBufferRHIRef ScratchVertexBufferRHI = tp_States->VertexBuffers.FindRef(clipDrawIndex);
            FTextureRHIRef tsr_TextureRHI = tp_Texture->Resource->TextureRHI;

            FillVertexBuffer(tp_Model, clipDrawIndex, ScratchVertexBufferRHI, tp_States, RHICmdList);

            ////////////////////////////////////////////////////////////////////////////

            FMatrix ts_MartixForMask;
            FVector4 ts_BaseColor;
            FVector4 ts_ChanelFlag;

            FillMaskParameter(clipContext, _clippingManager, ts_MartixForMask, ts_BaseColor, ts_ChanelFlag);

            VertexShader->SetParameters(RHICmdList, VertexShader.GetVertexShader(), ts_MartixForMask, ts_BaseColor, ts_ChanelFlag, tsr_TextureRHI);
            PixelShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), ts_MartixForMask, ts_BaseColor, ts_ChanelFlag, tsr_TextureRHI);
            SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

            ////////////////////////////////////////////////////////////////////////////

            RHICmdList.SetStreamSource(0, ScratchVertexBufferRHI, 0);

            RHICmdList.DrawIndexedPrimitive(
                IndexBufferRHI,
                /*BaseVertexIndex=*/ 0,
                /*MinIndex=*/ 0,
                /*NumVertices=*/ td_NumVertext,
                /*StartIndex=*/ 0,
                /*NumPrimitives=*/ indexCount / 3,
                /*NumInstances=*/ 1
            );



        }

    }
    RHICmdList.EndRenderPass();
}
