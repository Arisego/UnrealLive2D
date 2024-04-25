#include "ModelRenders.h"
#include "CubismSepRender.h"
#include "RHICommandList.h"
#include "RHIDefinitions.h"
#include "RHIResources.h"
#include "TextureResource.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "RHIStaticStates.h"
#include "Engine/Texture2D.h"
#include "PipelineStateCache.h"
#include "RenderUtils.h"
#include "ShaderParameterUtils.h"
#include "DataDrivenShaderPlatformInfo.h"


//////////////////////////////////////////////////////////////////////////
class FCubismSepShader : public FGlobalShader
{
    DECLARE_INLINE_TYPE_LAYOUT(FCubismSepShader, NonVirtual);

public:
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }

    static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    {
        FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
    }

    FCubismSepShader() {}

    FCubismSepShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer)
    {
        TestFloat.Bind(Initializer.ParameterMap, TEXT("TestFloat"));
        BaseColor.Bind(Initializer.ParameterMap, TEXT("BaseColor"));

        InTexture.Bind(Initializer.ParameterMap, TEXT("InTexture"));
        InTextureSampler.Bind(Initializer.ParameterMap, TEXT("InTextureSampler"));
    }

    template<typename TShaderRHIParamRef>
    void SetParameters(
        FRHICommandListImmediate& RHICmdList,
        const TShaderRHIParamRef ShaderRHI,
        const FVector4f& InBaseColor,
        FTextureRHIRef ShaderResourceTexture
    )
    {
        FRHIBatchedShaderParameters& BatchedParameters = RHICmdList.GetScratchShaderParameters();
        SetShaderValue(BatchedParameters, TestFloat, 1.0f);
        SetShaderValue(BatchedParameters, BaseColor, InBaseColor);
        
        SetTextureParameter(BatchedParameters, InTexture, ShaderResourceTexture);
        SetSamplerParameter(BatchedParameters, InTextureSampler, TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());

        RHICmdList.SetBatchedShaderParameters(ShaderRHI, BatchedParameters);
    }

private:
    LAYOUT_FIELD(FShaderParameter, TestFloat);
    LAYOUT_FIELD(FShaderParameter, BaseColor);

    LAYOUT_FIELD(FShaderResourceParameter, InTexture);
    LAYOUT_FIELD(FShaderResourceParameter, InTextureSampler);
};


class FCubismSepVS : public FCubismSepShader
{
    DECLARE_SHADER_TYPE(FCubismSepVS, Global);

public:

    /** Default constructor. */
    FCubismSepVS() {}

    /** Initialization constructor. */
    FCubismSepVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FCubismSepShader(Initializer)
    {
    }
};


class FCubismSepPS : public FCubismSepShader
{
    DECLARE_SHADER_TYPE(FCubismSepPS, Global);

public:

    /** Default constructor. */
    FCubismSepPS() {}

    /** Initialization constructor. */
    FCubismSepPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FCubismSepShader(Initializer)
    {


    }
private:

};

IMPLEMENT_SHADER_TYPE(, FCubismSepVS, TEXT("/Plugin/UnrealLive2D/Private/CubismSep.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FCubismSepPS, TEXT("/Plugin/UnrealLive2D/Private/CubismSep.usf"), TEXT("MainPS"), SF_Pixel)

//////////////////////////////////////////////////////////////////////////
using Csm::csmInt32;
using Csm::csmFloat32;
using Csm::csmUint16;
using Csm::csmBool;

void FModelRenders::DrawSepNormal(
    FTextureRenderTargetResource* OutTextureRenderTargetResource,
    FRHICommandListImmediate& RHICmdList,
    ERHIFeatureLevel::Type FeatureLevel,
    Csm::CubismModel* tp_Model,
    const Csm::csmInt32 drawableIndex,
    struct FCubismRenderState* tp_States
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
    check(indexCount > 0&& "Bad Index Count");

    FRHITexture2D* RenderTargetTexture = OutTextureRenderTargetResource->GetRenderTargetTexture();
    RHICmdList.Transition(MakeArrayView(FModelRenders::ConvertTransitionResource(FExclusiveDepthStencil::DepthWrite_StencilWrite, RenderTargetTexture)));

    FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::Load_Store);
    RHICmdList.BeginRenderPass(RPInfo, TEXT("DrawSeparate"));
    {
        FIntPoint DisplacementMapResolution(OutTextureRenderTargetResource->GetSizeX(), OutTextureRenderTargetResource->GetSizeY());

        // Update viewport.
        RHICmdList.SetViewport(
            0, 0, 0.f,
            DisplacementMapResolution.X, DisplacementMapResolution.Y, 1.f);

        // Get shaders.
        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
        TShaderMapRef< FCubismSepVS > VertexShader(GlobalShaderMap);
        TShaderMapRef< FCubismSepPS > PixelShader(GlobalShaderMap);

        // Set the graphic pipeline state.
        FGraphicsPipelineStateInitializer GraphicsPSOInit;
        RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
        GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
        GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
        GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None, ERasterizerDepthClipMode::DepthClip, true>::GetRHI();
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
        FBufferRHIRef IndexBufferRHI = tp_States->IndexBuffers.FindRef(drawableIndex);
        FBufferRHIRef ScratchVertexBufferRHI = tp_States->VertexBuffers.FindRef(drawableIndex);

        check(tp_Texture->GetResource());
        FTextureRHIRef tsr_TextureRHI = tp_Texture->GetResource()->TextureRHI;

        FillVertexBuffer(tp_Model, drawableIndex, ScratchVertexBufferRHI, tp_States, RHICmdList);

        //////////////////////////////////////////////////////////////////////////
        // TODO: ModelColor
        FVector4f ts_BaseColor = FVector4f(1.0f, 1.0f, 1.0f, 1.0f);
        ts_BaseColor.W = tf_Opacity;

        GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
        GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
        SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

        VertexShader->SetParameters(RHICmdList, VertexShader.GetVertexShader(), ts_BaseColor, tsr_TextureRHI);
        PixelShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), ts_BaseColor, tsr_TextureRHI);

        //////////////////////////////////////////////////////////////////////////

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
    RHICmdList.EndRenderPass();
}


void FModelRenders::DrawTestTexture(FTextureRenderTargetResource* OutTextureRenderTargetResource, FRHICommandListImmediate& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, struct FCubismRenderState* tp_States)
{
    FRHITexture2D* RenderTargetTexture = OutTextureRenderTargetResource->GetRenderTargetTexture();
    RHICmdList.Transition(MakeArrayView(FModelRenders::ConvertTransitionResource(FExclusiveDepthStencil::DepthWrite_StencilWrite, RenderTargetTexture)));

    FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::Clear_Store);
    RHICmdList.BeginRenderPass(RPInfo, TEXT("DrawSeparateTest"));
    {

        FIntPoint DisplacementMapResolution(OutTextureRenderTargetResource->GetSizeX(), OutTextureRenderTargetResource->GetSizeY());

        // Update viewport.
        RHICmdList.SetViewport(
            0, 0, 0.f,
            DisplacementMapResolution.X, DisplacementMapResolution.Y, 1.f);

        // Get shaders.
        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
        TShaderMapRef< FCubismSepVS > VertexShader(GlobalShaderMap);
        TShaderMapRef< FCubismSepPS > PixelShader(GlobalShaderMap);

        // Set the graphic pipeline state.
        FGraphicsPipelineStateInitializer GraphicsPSOInit;
        RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
        GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
        GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
        GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None, ERasterizerDepthClipMode::DepthClip, true>::GetRHI();
        GraphicsPSOInit.PrimitiveType = PT_TriangleList;
        GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GCubismVertexDeclaration.VertexDeclarationRHI;


        // Update viewport.
        RHICmdList.SetViewport(
            0, 0, 0.f,
            OutTextureRenderTargetResource->GetSizeX(), OutTextureRenderTargetResource->GetSizeY(), 1.f);

        // Shader
        GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
        GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

        SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

        FVector4f ts_FakeBase = FVector4f(1.0f, 1.0f, 1.0f, 1.0f);
        VertexShader->SetParameters(RHICmdList, VertexShader.GetVertexShader(), ts_FakeBase, tp_States->MaskBuffer->GetTexture2D());
        PixelShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), ts_FakeBase, tp_States->MaskBuffer->GetTexture2D());

        // Texture
        //UTexture2D* tp_Texture = tp_States->Textures[0];
        //FTextureRHIRef tsr_TextureRHI = tp_Texture->Resource->TextureRHI;
        //PixelShader->SetInTexture(RHICmdList, tsr_TextureRHI);


        RHICmdList.SetStreamSource(0, GCubismVertexScreenBuffer.VertexBufferRHI, 0);
        RHICmdList.DrawIndexedPrimitive(GTwoTrianglesIndexBuffer.IndexBufferRHI, 0, 0, 4, 0, 2, 1);
    }
    RHICmdList.EndRenderPass();
}

