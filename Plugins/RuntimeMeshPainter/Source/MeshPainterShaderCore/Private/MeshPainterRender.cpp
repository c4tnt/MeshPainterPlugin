#include "MeshPainterRender.h"
#include "MeshPainterShader.h"
#include "MeshPassProcessor.h"
#include "MeshBatch.h"
#include "PrimitiveSceneInfo.h"
#include "Materials/MaterialRenderProxy.h"
#include "RenderGraphBuilder.h"
#include "EngineModule.h"
#include "SceneRendererInterface.h"
#include "InstanceCulling/InstanceCullingContext.h"
#include "RenderCaptureInterface.h"
#include "MeshPassProcessor.inl"

#if (!UE_BUILD_SHIPPING && !UE_BUILD_TEST)
static int32 RenderCaptureDraws = 0;
static FAutoConsoleVariableRef CVarRenderCaptureDraws(
	TEXT("r.MeshPaintPass.RenderCaptureDraws"),
	RenderCaptureDraws,
	TEXT("Enable capturing of render capture texture for the next N mesh paint passes"));
#endif

class FMeshPaintPassProcessor : public FMeshPassProcessor
{
public:
	FMeshPaintPassProcessor(const FSceneView* InView, FMeshPassDrawListContext* InDrawListContext, const TArray<FMeshPaintProxyRenderParameters>& PrimitiveInfos, const FMaterialRenderProxy* InMaterial, EMeshPaintShaderOutputBits InOutputs)
		: FMeshPassProcessor(EMeshPass::Num, nullptr, GMaxRHIFeatureLevel, InView, InDrawListContext), MaterialOverride(InMaterial), ActiveOutputs(InOutputs)
	{
		DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
		DrawRenderState.SetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI());

		for (const FMeshPaintProxyRenderParameters& Params : PrimitiveInfos)
		{
			PrimitiveDetails.Add(Params.PrimitiveProxy, FPrimitiveDetails(Params.UVRegion));
		}
	}

	virtual void AddMeshBatch(const FMeshBatch& RESTRICT MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, int32 StaticMeshId = -1) override final
	{
		const FMaterialRenderProxy* SourceMaterialRenderProxy = MaterialOverride ? MaterialOverride : MeshBatch.MaterialRenderProxy;
		const FMaterialRenderProxy* FallbackMaterialRenderProxy = nullptr;
		const FMaterial& Material = SourceMaterialRenderProxy->GetMaterialWithFallback(FeatureLevel, FallbackMaterialRenderProxy);
		const FMaterialRenderProxy& MaterialRenderProxy = FallbackMaterialRenderProxy ? *FallbackMaterialRenderProxy : *SourceMaterialRenderProxy;
		const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;

		MaterialRenderProxy.UpdateUniformExpressionCacheIfNeeded(GMaxRHIFeatureLevel);

		const FPrimitiveDetails* PrimitiveUVInfo = PrimitiveDetails.Find(PrimitiveSceneProxy);

		if (!PrimitiveUVInfo)
		{
			return;
		}

		TMeshProcessorShaders<FMeshPaintShaderVS, FMeshPaintShaderPS> PassShaders;

		FMeshPaintShaderPS::FPermutationDomain PSPremutation;
		PSPremutation.Set<FMeshPaintShaderPS::FOutputBits>((int32)ActiveOutputs);

		FMaterialShaderTypes ShaderTypes;
		ShaderTypes.AddShaderType<FMeshPaintShaderVS>();
		ShaderTypes.AddShaderType<FMeshPaintShaderPS>(PSPremutation.ToDimensionValueId());

		FMaterialShaders Shaders;
		if (!Material.TryGetShaders(ShaderTypes, VertexFactory->GetType(), Shaders))
		{
			return;
		}

		Shaders.TryGetVertexShader(PassShaders.VertexShader);
		Shaders.TryGetPixelShader(PassShaders.PixelShader);

		FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(MeshBatch);
		ERasterizerFillMode MeshFillMode = ComputeMeshFillMode(Material, OverrideSettings);
		ERasterizerCullMode MeshCullMode = CM_None; //ComputeMeshCullMode(Material, OverrideSettings);

		FMeshPaintShaderElementData ShaderElementData;
		ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, false);
		
		const FVector2D UVScale = PrimitiveUVInfo->UVRegion.GetSize();
		const FVector2D UVBias = PrimitiveUVInfo->UVRegion.Min;
		ShaderElementData.UVTileMapping = FVector4(UVScale * FVector2D(2.0f, -2.0f), UVBias * 2.0f + FVector2D(-1.0f, 1.0f));

		FMeshDrawCommandSortKey SortKey = CreateMeshSortKey(MeshBatch, PrimitiveSceneProxy, Material, PassShaders.VertexShader.GetShader(), PassShaders.PixelShader.GetShader());

		BuildMeshDrawCommands(
			MeshBatch,
			BatchElementMask,
			PrimitiveSceneProxy,
			MaterialRenderProxy,
			Material,
			DrawRenderState,
			PassShaders,
			MeshFillMode,
			MeshCullMode,
			SortKey,
			EMeshPassFeatures::Default,
			ShaderElementData);
	}

protected:
	virtual FMeshDrawCommandSortKey CreateMeshSortKey(const FMeshBatch& RESTRICT MeshBatch,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		const FMaterial& Material,
		const FMeshMaterialShader* VertexShader,
		const FMeshMaterialShader* PixelShader)
	{
		FMeshDrawCommandSortKey SortKey = FMeshDrawCommandSortKey::Default;

		uint16 SortKeyPriority = 0;
		float Distance = 0.0f;

		if (PrimitiveSceneProxy)
		{
			const FPrimitiveSceneInfo* PrimitiveSceneInfo = PrimitiveSceneProxy->GetPrimitiveSceneInfo();
			SortKeyPriority = (uint16)((int32)PrimitiveSceneInfo->Proxy->GetTranslucencySortPriority() - (int32)SHRT_MIN);

			// Use the standard sort by distance method for translucent objects
			const float DistanceOffset = PrimitiveSceneInfo->Proxy->GetTranslucencySortDistanceOffset();
			const FVector BoundsOrigin = PrimitiveSceneProxy->GetBounds().Origin;
			const FVector ViewOrigin = this->ViewIfDynamicMeshCommand->ViewMatrices.GetViewOrigin();
			Distance = (BoundsOrigin - ViewOrigin).Size() + DistanceOffset;
		}

		SortKey.Translucent.MeshIdInPrimitive = MeshBatch.MeshIdInPrimitive;
		SortKey.Translucent.Priority = SortKeyPriority;
		SortKey.Translucent.Distance = (uint32)~BitInvertIfNegativeFloat(*(uint32*)&Distance);

		return SortKey;
	}

private:
	/** Inverts the bits of the floating point number if that number is negative */
	uint32 BitInvertIfNegativeFloat(uint32 FloatBit)
	{
		unsigned Mask = -int32(FloatBit >> 31) | 0x80000000;
		return FloatBit ^ Mask;
	}

private:
	const FMaterialRenderProxy* MaterialOverride;
	FMeshPassProcessorRenderState DrawRenderState;
	EMeshPaintShaderOutputBits ActiveOutputs;
	struct FPrimitiveDetails
	{
		FBox2D UVRegion;
	};
	TMap<FPrimitiveSceneProxy*, FPrimitiveDetails> PrimitiveDetails;
};

bool MeshPaintRender::AddMeshPaintPass(FRHICommandListImmediate& RHICmdList, const FMeshPaintRenderTargets& InRenderTargets, const FMeshPaintRenderParameters& InParameters)
{
	FRDGBuilder GraphBuilder(RHICmdList);
	bool bResult = AddMeshPaintPass(GraphBuilder, InRenderTargets, InParameters);
	GraphBuilder.Execute();
	return bResult;
}

bool MeshPaintRender::AddMeshPaintPass(FRDGBuilder& GraphBuilder, const FMeshPaintRenderTargets& InRenderTargets, const FMeshPaintRenderParameters& InParameters)
{
	check(IsInRenderingThread());

	if (!InRenderTargets.IsValidForRendering() || InParameters.PrimitivesToRender.IsEmpty())
	{
		return false;
	}

#if (!UE_BUILD_SHIPPING && !UE_BUILD_TEST)
	RenderCaptureInterface::FScopedCapture RenderCapture(RenderCaptureDraws > 0, GraphBuilder);
	RenderCaptureDraws = FMath::Max(0, RenderCaptureDraws - 1);
#endif

	FEngineShowFlags EngineShowFlags(ESFIM_Game);

	// LightCard settings from the FDisplayClusterViewportManager::ConfigureViewFamily
	{
		EngineShowFlags.PostProcessing = 0;
		EngineShowFlags.SetAtmosphere(0);
		EngineShowFlags.SetFog(0);
		EngineShowFlags.SetVolumetricFog(0);
		EngineShowFlags.SetMotionBlur(0); // motion blur doesn't work correctly with scene captures.
		EngineShowFlags.SetSeparateTranslucency(0);
		EngineShowFlags.SetHMDDistortion(0);
		EngineShowFlags.SetOnScreenDebug(0);

		EngineShowFlags.SetLumenReflections(0);
		EngineShowFlags.SetLumenGlobalIllumination(0);
		EngineShowFlags.SetGlobalIllumination(0);

		EngineShowFlags.SetScreenSpaceAO(0);
		EngineShowFlags.SetAmbientOcclusion(0);
		EngineShowFlags.SetDeferredLighting(0);
		EngineShowFlags.SetVirtualTexturePrimitives(0);
		EngineShowFlags.SetRectLights(0);
	}

	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		InRenderTargets.GetPrimaryRenderTarget(),
		InParameters.Scene,
		EngineShowFlags)
		.SetTime(FGameTime::GetTimeSinceAppStart())
		.SetGammaCorrection(1.0f));

	FScenePrimitiveRenderingContextScopeHelper ScenePrimitiveRenderingContextScopeHelper(GetRendererModule().BeginScenePrimitiveRendering(GraphBuilder, &ViewFamily));

	FIntPoint ViewSize = InRenderTargets.GetPrimaryRenderTarget()->GetSizeXY();
	
	FSceneViewInitOptions ViewInitOptions;
	*static_cast<FSceneViewProjectionData*>(&ViewInitOptions) = InParameters.ViewProjection;
	ViewInitOptions.ViewFamily = &ViewFamily;
	ViewInitOptions.SetViewRectangle(FIntRect(FIntPoint::ZeroValue, ViewSize));
	ViewInitOptions.bIsSceneCapture = true;

	GetRendererModule().CreateAndInitSingleView(GraphBuilder.RHICmdList, &ViewFamily, &ViewInitOptions);
	FSceneView* View = (FSceneView*)ViewFamily.Views[0];

	ViewFamily.EngineShowFlags.SetToneCurve(false);

	// This flags sets tonemapper to output to ETonemapperOutputDevice::LinearNoToneCurve
	View->FinalPostProcessSettings.bOverride_ToneCurveAmount = 1;
	View->FinalPostProcessSettings.ToneCurveAmount = 0.0;

	FMeshPaintShaderParameters* PassParameters = GraphBuilder.AllocParameters<FMeshPaintShaderParameters>();
	PassParameters->View = View->ViewUniformBuffer;
	PassParameters->Scene = GetSceneUniformBufferRef(GraphBuilder, *View);
	PassParameters->InstanceCulling = FInstanceCullingContext::CreateDummyInstanceCullingUniformBuffer(GraphBuilder);

	const bool bClearTargets = InParameters.bClearTargets;
	EMeshPaintShaderOutputBits ActiveOutputs = EMeshPaintShaderOutputBits::None;
	int32 MRTIndex = 0;
	if (InRenderTargets.HasTargetOfType(FMeshPaintRenderTargets::RT_BaseColor))
	{
		FRDGTextureRef OutputTexture = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(InRenderTargets.BaseColor->GetRenderTargetTexture(), TEXT("MeshPaintBCOutputTexture")));
		PassParameters->RenderTargets[MRTIndex] = FRenderTargetBinding(OutputTexture, bClearTargets ? ERenderTargetLoadAction::EClear : ERenderTargetLoadAction::ELoad);
		ActiveOutputs |= EMeshPaintShaderOutputBits::BaseColor;
		MRTIndex++;
	}
	if (InRenderTargets.HasTargetOfType(FMeshPaintRenderTargets::RT_Emissive))
	{
		FRDGTextureRef OutputTexture = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(InRenderTargets.Emissive->GetRenderTargetTexture(), TEXT("MeshPaintEmissiveOutputTexture")));
		PassParameters->RenderTargets[MRTIndex] = FRenderTargetBinding(OutputTexture, bClearTargets ? ERenderTargetLoadAction::EClear : ERenderTargetLoadAction::ELoad);
		ActiveOutputs |= EMeshPaintShaderOutputBits::Emissive;
		MRTIndex++;
	}
	if (InRenderTargets.HasTargetOfType(FMeshPaintRenderTargets::RT_NormalMap))
	{
		FRDGTextureRef OutputTexture = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(InRenderTargets.NormalMap->GetRenderTargetTexture(), TEXT("MeshPaintNormalOutputTexture")));
		PassParameters->RenderTargets[MRTIndex] = FRenderTargetBinding(OutputTexture, bClearTargets ? ERenderTargetLoadAction::EClear : ERenderTargetLoadAction::ELoad);
		ActiveOutputs |= EMeshPaintShaderOutputBits::Normal;
		MRTIndex++;
	}

	GraphBuilder.AddPass(RDG_EVENT_NAME("MeshPaintRender::MeshPaintPass %dx%d", ViewSize.X, ViewSize.Y),
		PassParameters,
		ERDGPassFlags::Raster | ERDGPassFlags::NeverCull,
		[=, &InParameters](FRHICommandList& RHICmdList)
		{
			FIntRect ViewRect = View->UnscaledViewRect;
			RHICmdList.SetViewport(ViewRect.Min.X, ViewRect.Min.Y, 0.0f, ViewRect.Max.X, ViewRect.Max.Y, 1.0f);

			DrawDynamicMeshPass(*View, RHICmdList, [=, &InParameters](FDynamicPassMeshDrawListContext* DynamicMeshPassContext)
			{
				FMeshPaintPassProcessor MeshPassProcessor(View, DynamicMeshPassContext, InParameters.PrimitivesToRender, InParameters.MaterialOverride, ActiveOutputs);
				for (const FMeshPaintProxyRenderParameters& PrimitiveInfo : InParameters.PrimitivesToRender)
				{
					//PrimitiveInfo.PrimitiveProxy->DrawStaticElements();

					FPrimitiveSceneInfo* PrimitiveSceneInfo = PrimitiveInfo.PrimitiveProxy->GetPrimitiveSceneInfo();
					const uint8 MaxLOD = PrimitiveSceneInfo->StaticMeshes.Num() - 1;
					const uint8 MinLOD = PrimitiveInfo.PrimitiveProxy->GetCurrentFirstLODIdx_RenderThread();
					const uint8 RenderLOD = FMath::Clamp(PrimitiveInfo.TargetLOD, MinLOD, MaxLOD);

					if (const FMeshBatch* MeshBatch = PrimitiveSceneInfo->GetMeshBatch(RenderLOD))
					{
						const uint64 BatchElementMask = ~0ull;
						MeshPassProcessor.AddMeshBatch(*MeshBatch, BatchElementMask, PrimitiveInfo.PrimitiveProxy);
					}
				}
			});
		});

	return true;
}
