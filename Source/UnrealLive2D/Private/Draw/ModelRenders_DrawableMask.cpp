#include "ModelRenders.h"
#include "TextureResource.h"
#include "FWPort/ModelRender.h"
#include "GlobalShader.h"
#include "RHIStaticStates.h"
#include "CubismSepRender.h"
#include "Engine/Texture2D.h"
#include "PipelineStateCache.h"

#include "UnrealLive2D.h"
#include "ShaderParameterUtils.h"

//////////////////////////////////////////////////////////////////////////

class FCubismSepMask : public FGlobalShader
{
    DECLARE_INLINE_TYPE_LAYOUT(FCubismSepMask, NonVirtual);

public:
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }

    static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    {
        FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
    }

    FCubismSepMask() {}

    FCubismSepMask(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer)
    {
        TestFloat.Bind(Initializer.ParameterMap, TEXT("TestFloat"));
        ProjectMatrix.Bind(Initializer.ParameterMap, TEXT("ProjectMatrix"));
        ClipMatrix.Bind(Initializer.ParameterMap, TEXT("ClipMatrix"));
        BaseColor.Bind(Initializer.ParameterMap, TEXT("BaseColor"));
        ChannelFlag.Bind(Initializer.ParameterMap, TEXT("ChannelFlag"));

        MainTexture.Bind(Initializer.ParameterMap, TEXT("MainTexture"));
        MaskTexture.Bind(Initializer.ParameterMap, TEXT("MaskTexture"));
        MainTextureSampler.Bind(Initializer.ParameterMap, TEXT("MainSampler"));
        MaskSampler.Bind(Initializer.ParameterMap, TEXT("MaskSampler"));
    }

    template<typename TShaderRHIParamRef>
    void SetParameters(
        FRHICommandListImmediate& RHICmdList,
        const TShaderRHIParamRef ShaderRHI,
        const FMatrix& InProjectMatrix,
        const FMatrix& InClipMatrix,
        const FVector4& InBaseColor,
        const FVector4& InChannelFlag,
        FTextureRHIRef MainTextureRef,
        FTextureRHIRef MaskTextureRef
    )
    {
        SetShaderValue(RHICmdList, ShaderRHI, TestFloat, 1.0f);
        SetShaderValue(RHICmdList, ShaderRHI, ProjectMatrix, InProjectMatrix);
        SetShaderValue(RHICmdList, ShaderRHI, ClipMatrix, InClipMatrix);
        SetShaderValue(RHICmdList, ShaderRHI, BaseColor, InBaseColor);
        SetShaderValue(RHICmdList, ShaderRHI, ChannelFlag, InChannelFlag);

        SetTextureParameter(RHICmdList, ShaderRHI, MainTexture, MainTextureRef);
        SetTextureParameter(RHICmdList, ShaderRHI, MaskTexture, MaskTextureRef);
        SetSamplerParameter(RHICmdList, ShaderRHI, MainTextureSampler, TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());
        SetSamplerParameter(RHICmdList, ShaderRHI, MaskSampler, TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());
    }

private:
    LAYOUT_FIELD(FShaderParameter, TestFloat);
    LAYOUT_FIELD(FShaderParameter, ProjectMatrix);
    LAYOUT_FIELD(FShaderParameter, ClipMatrix);
    LAYOUT_FIELD(FShaderParameter, BaseColor);
    LAYOUT_FIELD(FShaderParameter, ChannelFlag);

    LAYOUT_FIELD(FShaderResourceParameter, MainTexture);
    LAYOUT_FIELD(FShaderResourceParameter, MaskTexture);
    LAYOUT_FIELD(FShaderResourceParameter, MainTextureSampler);
    LAYOUT_FIELD(FShaderResourceParameter, MaskSampler);
};

class FCubismSepMaskVS : public FCubismSepMask
{
    DECLARE_SHADER_TYPE(FCubismSepMaskVS, Global);

public:

    /** Default constructor. */
    FCubismSepMaskVS() {}

    /** Initialization constructor. */
    FCubismSepMaskVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FCubismSepMask(Initializer)
    {
    }

private:
};

class FCubismSepMaskPS : public FCubismSepMask
{
    DECLARE_SHADER_TYPE(FCubismSepMaskPS, Global);

public:

    /** Default constructor. */
    FCubismSepMaskPS() {}

    /** Initialization constructor. */
    FCubismSepMaskPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FCubismSepMask(Initializer)
    {

    }
};

IMPLEMENT_SHADER_TYPE(, FCubismSepMaskVS, TEXT("/Plugin/UnrealLive2D/Private/CubismSepMask.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FCubismSepMaskPS, TEXT("/Plugin/UnrealLive2D/Private/CubismSepMask.usf"), TEXT("MainPS"), SF_Pixel)

class FCubismSepMaskPS_Invert : public FCubismSepMask
{
    DECLARE_SHADER_TYPE(FCubismSepMaskPS_Invert, Global);

public:

    /** Default constructor. */
    FCubismSepMaskPS_Invert() {}

    /** Initialization constructor. */
    FCubismSepMaskPS_Invert(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FCubismSepMask(Initializer)
    {

    }
};

IMPLEMENT_SHADER_TYPE(, FCubismSepMaskPS_Invert, TEXT("/Plugin/UnrealLive2D/Private/CubismSepMask.usf"), TEXT("MainPsInvert"), SF_Pixel)

//////////////////////////////////////////////////////////////////////////
void FModelRenders::DrawSepMask(
    FTextureRenderTargetResource* OutTextureRenderTargetResource,
    FRHICommandListImmediate& RHICmdList,
    ERHIFeatureLevel::Type FeatureLevel,
    Csm::CubismModel* tp_Model,
    const csmInt32 drawableIndex,
    FCubismRenderState* tp_States,
    CubismClippingContext* clipContext
)
{
    if (!tp_Model)
    {
        UE_LOG(LogCubism, Error, TEXT("FModelRenders::DrawSepMask: Model not valid"));
        return;
    }

    if (tp_Model->GetDrawableInvertedMask(drawableIndex))
    {
        _DrawSepMask_Invert(OutTextureRenderTargetResource, RHICmdList, FeatureLevel, tp_Model, drawableIndex, tp_States, clipContext);
    }
    else
    {
        _DrawSepMask_Normal(OutTextureRenderTargetResource, RHICmdList, FeatureLevel, tp_Model, drawableIndex, tp_States, clipContext);
    }
}

void FModelRenders::_DrawSepMask_Normal(
    FTextureRenderTargetResource* OutTextureRenderTargetResource,
    FRHICommandListImmediate& RHICmdList, 
    ERHIFeatureLevel::Type FeatureLevel, 
    Csm::CubismModel* tp_Model, 
    const csmInt32 drawableIndex, 
    FCubismRenderState* tp_States, 
    CubismClippingContext* clipContext
)
{

    csmFloat32 tf_Opacity = tp_Model->GetDrawableOpacity(drawableIndex);
    if (tf_Opacity <= 0.0)
    {
        return;
    }

    UTexture2D* tp_Texture = GetTexture(tp_Model, drawableIndex, tp_States);
    if (!tp_Texture)
    {
        return;
    }

    const csmInt32 indexCount = tp_Model->GetDrawableVertexIndexCount(drawableIndex);
    check(indexCount > 0 && "Bad Index Count");

    //////////////////////////////////////////////////////////////////////////
    FRHITexture2D* RenderTargetTexture = OutTextureRenderTargetResource->GetRenderTargetTexture();
    RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, RenderTargetTexture);

    FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::Load_Store, OutTextureRenderTargetResource->TextureRHI);
    RHICmdList.BeginRenderPass(RPInfo, TEXT("DrawSeparate"));
    {
        FIntPoint DisplacementMapResolution(OutTextureRenderTargetResource->GetSizeX(), OutTextureRenderTargetResource->GetSizeY());

        // Update viewport.
        RHICmdList.SetViewport(
            0, 0, 0.f,
            DisplacementMapResolution.X, DisplacementMapResolution.Y, 1.f);

        // Get shaders.
        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
        TShaderMapRef< FCubismSepMaskVS > VertexShader_Mask(GlobalShaderMap);
        TShaderMapRef< FCubismSepMaskPS > PixelShader_Mask(GlobalShaderMap);

        // Set the graphic pipeline state.
        FGraphicsPipelineStateInitializer GraphicsPSOInit;
        RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
        GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
        GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
        GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
        GraphicsPSOInit.PrimitiveType = PT_TriangleList;
        GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GCubismVertexDeclaration.VertexDeclarationRHI;


        // Update viewport.
        RHICmdList.SetViewport(
            0, 0, 0.f,
            OutTextureRenderTargetResource->GetSizeX(), OutTextureRenderTargetResource->GetSizeY(), 1.f);

        //////////////////////////////////////////////////////////////////////////
        const csmInt32 td_NumVertext = tp_Model->GetDrawableVertexCount(drawableIndex);
        csmBool tb_InvertMask = tp_Model->GetDrawableInvertedMask(drawableIndex);
        SetUpBlendMode(tp_Model, drawableIndex, GraphicsPSOInit);

        //////////////////////////////////////////////////////////////////////////
        /** Drawable draw */
        FIndexBufferRHIRef IndexBufferRHI = tp_States->IndexBuffers.FindRef(drawableIndex);
        FVertexBufferRHIRef ScratchVertexBufferRHI = tp_States->VertexBuffers.FindRef(drawableIndex);
        FTextureRHIRef tsr_TextureRHI = tp_Texture->Resource->TextureRHI;

        FillVertexBuffer(tp_Model, drawableIndex, ScratchVertexBufferRHI, tp_States, RHICmdList);

        //////////////////////////////////////////////////////////////////////////
        // TODO: ModelColor
        FVector4 ts_BaseColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
        ts_BaseColor.W = tf_Opacity;

        /** TODO: If find some model use this */
        check(!tb_InvertMask);

        GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader_Mask.GetVertexShader();
        GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader_Mask.GetPixelShader();

        static FMatrix ts_FakeGlobal = FMatrix::Identity;
        FMatrix ts_MartixForDraw;
        FVector4 ts_ChanelFlag;

        {
            // チャンネル
            const csmInt32 channelNo = clipContext->_layoutChannelNo;
            // チャンネルをRGBAに変換
            Csm::Rendering::CubismRenderer::CubismTextureColor* colorChannel = tp_States->_ClippingManager->GetChannelFlagAsColor(channelNo);

            csmRectF* rect = clipContext->_layoutBounds;

            ts_MartixForDraw = ConvertCubismMatrix(clipContext->_matrixForDraw);
            ts_ChanelFlag = FVector4(colorChannel->R, colorChannel->G, colorChannel->B, colorChannel->A);
        }

        UE_LOG(LogCubism, Verbose, TEXT("_matrixForDraw: %s"), *ts_MartixForDraw.ToString());
        UE_LOG(LogCubism, Verbose, TEXT("_matrixForMask: %s"), *ConvertCubismMatrix(clipContext->_matrixForMask).ToString());

        FTexture2DRHIRef tsr_MaskBuffer = tp_States->MaskBuffer;
        FTextureRHIRef tsr_MaskTexture = tsr_MaskBuffer->GetTexture2D();

        VertexShader_Mask->SetParameters(RHICmdList, VertexShader_Mask.GetVertexShader(), ts_MartixForDraw, ts_MartixForDraw, ts_BaseColor, ts_ChanelFlag, tsr_TextureRHI, tsr_MaskTexture);
        PixelShader_Mask->SetParameters(RHICmdList, PixelShader_Mask.GetPixelShader(), ts_MartixForDraw, ts_MartixForDraw, ts_BaseColor, ts_ChanelFlag, tsr_TextureRHI, tsr_MaskTexture);

        //////////////////////////////////////////////////////////////////////////
        SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);


        RHICmdList.SetStreamSource(0, ScratchVertexBufferRHI, 0);
        //RHICmdList.DrawIndexedPrimitive(IndexBufferRHI, 0, 0, 4, 0, 2, 1);

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
    RHICmdList.EndRenderPass();
 }

 void FModelRenders::_DrawSepMask_Invert(
     FTextureRenderTargetResource* OutTextureRenderTargetResource,
     FRHICommandListImmediate& RHICmdList,
     ERHIFeatureLevel::Type FeatureLevel,
     Csm::CubismModel* tp_Model,
     const csmInt32 drawableIndex,
     FCubismRenderState* tp_States,
     CubismClippingContext* clipContext
 )
 {

     csmFloat32 tf_Opacity = tp_Model->GetDrawableOpacity(drawableIndex);
     if (tf_Opacity <= 0.0)
     {
         return;
     }

     UTexture2D* tp_Texture = GetTexture(tp_Model, drawableIndex, tp_States);
     if (!tp_Texture)
     {
         return;
     }

     const csmInt32 indexCount = tp_Model->GetDrawableVertexIndexCount(drawableIndex);
     check(indexCount > 0 && "Bad Index Count");

     //////////////////////////////////////////////////////////////////////////
     FRHITexture2D* RenderTargetTexture = OutTextureRenderTargetResource->GetRenderTargetTexture();
     RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, RenderTargetTexture);

     FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::Load_Store, OutTextureRenderTargetResource->TextureRHI);
     RHICmdList.BeginRenderPass(RPInfo, TEXT("DrawSeparate"));
     {
         FIntPoint DisplacementMapResolution(OutTextureRenderTargetResource->GetSizeX(), OutTextureRenderTargetResource->GetSizeY());

         // Update viewport.
         RHICmdList.SetViewport(
             0, 0, 0.f,
             DisplacementMapResolution.X, DisplacementMapResolution.Y, 1.f);

         // Get shaders.
         FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
         TShaderMapRef< FCubismSepMaskVS > VertexShader_Mask(GlobalShaderMap);
         TShaderMapRef< FCubismSepMaskPS_Invert> PixelShader_Mask_Invert(GlobalShaderMap);

         // Set the graphic pipeline state.
         FGraphicsPipelineStateInitializer GraphicsPSOInit;
         RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
         GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
         GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
         GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
         GraphicsPSOInit.PrimitiveType = PT_TriangleList;
         GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GCubismVertexDeclaration.VertexDeclarationRHI;


         // Update viewport.
         RHICmdList.SetViewport(
             0, 0, 0.f,
             OutTextureRenderTargetResource->GetSizeX(), OutTextureRenderTargetResource->GetSizeY(), 1.f);

         //////////////////////////////////////////////////////////////////////////
         const csmInt32 td_NumVertext = tp_Model->GetDrawableVertexCount(drawableIndex);
         SetUpBlendMode(tp_Model, drawableIndex, GraphicsPSOInit);

         //////////////////////////////////////////////////////////////////////////
         /** Drawable draw */
         FIndexBufferRHIRef IndexBufferRHI = tp_States->IndexBuffers.FindRef(drawableIndex);
         FVertexBufferRHIRef ScratchVertexBufferRHI = tp_States->VertexBuffers.FindRef(drawableIndex);
         FTextureRHIRef tsr_TextureRHI = tp_Texture->Resource->TextureRHI;

         FillVertexBuffer(tp_Model, drawableIndex, ScratchVertexBufferRHI, tp_States, RHICmdList);

         //////////////////////////////////////////////////////////////////////////
         // TODO: ModelColor
         FVector4 ts_BaseColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
         ts_BaseColor.W = tf_Opacity;

         GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader_Mask.GetVertexShader();
         GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader_Mask_Invert.GetPixelShader();


         static FMatrix ts_FakeGlobal = FMatrix::Identity;
         FMatrix ts_MartixForDraw;
         FVector4 ts_ChanelFlag;

         {
             // チャンネル
             const csmInt32 channelNo = clipContext->_layoutChannelNo;
             // チャンネルをRGBAに変換
             Csm::Rendering::CubismRenderer::CubismTextureColor* colorChannel = tp_States->_ClippingManager->GetChannelFlagAsColor(channelNo);

             csmRectF* rect = clipContext->_layoutBounds;

             ts_MartixForDraw = ConvertCubismMatrix(clipContext->_matrixForDraw);
             ts_ChanelFlag = FVector4(colorChannel->R, colorChannel->G, colorChannel->B, colorChannel->A);
         }

         UE_LOG(LogCubism, Verbose, TEXT("_matrixForDraw: %s"), *ts_MartixForDraw.ToString());
         UE_LOG(LogCubism, Verbose, TEXT("_matrixForMask: %s"), *ConvertCubismMatrix(clipContext->_matrixForMask).ToString());

         FTexture2DRHIRef tsr_MaskBuffer = tp_States->MaskBuffer;
         FTextureRHIRef tsr_MaskTexture = tsr_MaskBuffer->GetTexture2D();

         VertexShader_Mask->SetParameters(RHICmdList, VertexShader_Mask.GetVertexShader(), ts_MartixForDraw, ts_MartixForDraw, ts_BaseColor, ts_ChanelFlag, tsr_TextureRHI, tsr_MaskTexture);
         PixelShader_Mask_Invert->SetParameters(RHICmdList, PixelShader_Mask_Invert.GetPixelShader(), ts_MartixForDraw, ts_MartixForDraw, ts_BaseColor, ts_ChanelFlag, tsr_TextureRHI, tsr_MaskTexture);



         //////////////////////////////////////////////////////////////////////////
         SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);


         RHICmdList.SetStreamSource(0, ScratchVertexBufferRHI, 0);
         //RHICmdList.DrawIndexedPrimitive(IndexBufferRHI, 0, 0, 4, 0, 2, 1);

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
     RHICmdList.EndRenderPass();
 }
