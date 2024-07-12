// Fill out your copyright notice in the Description page of Project Settings.

#include "MeshPainterFunctionLibrary.h"
#include "PrimitiveSceneInfo.h"
//#include "StaticMeshBatch.h"
#include "MeshPainterRender.h"

bool UMeshPainterFunctionLibrary::RenderMaterialOnMeshUVLayout(
	UObject* WorldContextObject,
	UPrimitiveComponent* MeshComponent, 
	UMaterialInterface* Material, 
	UTextureRenderTarget2D* BaseColor, 
	UTextureRenderTarget2D* Emissive,
	UTextureRenderTarget2D* NormalMap,
	int32 LOD, bool bClearRenderTargets)
{
	FRenderMaterialOnMeshPrimitive PrimitiveInfo;
	PrimitiveInfo.DesiredLOD = LOD;
	PrimitiveInfo.MeshComponent = MeshComponent;
	return RenderMaterialOnMeshUVAtlasMulti(WorldContextObject, MakeArrayView(&PrimitiveInfo, 1), Material, BaseColor, Emissive, NormalMap, FRenderMaterialOnMeshViewConfiguration(), bClearRenderTargets);
}

bool UMeshPainterFunctionLibrary::RenderMaterialOnMeshUVAtlas(
	UObject* WorldContextObject,
	UPrimitiveComponent* MeshComponent, 
	UMaterialInterface* Material, 
	UTextureRenderTarget2D* BaseColor, 
	UTextureRenderTarget2D* Emissive,
	UTextureRenderTarget2D* NormalMap,
	int32 LOD, const FBox2D& UVRegion, bool bClearRenderTargets)
{
	FRenderMaterialOnMeshPrimitive PrimitiveInfo;
	PrimitiveInfo.DesiredLOD = LOD;
	PrimitiveInfo.MeshComponent = MeshComponent;
	PrimitiveInfo.UVRegion = UVRegion;
	return RenderMaterialOnMeshUVAtlasMulti(WorldContextObject, MakeArrayView(&PrimitiveInfo, 1), Material, BaseColor, Emissive, NormalMap, FRenderMaterialOnMeshViewConfiguration(), bClearRenderTargets);
}

bool UMeshPainterFunctionLibrary::RenderMaterialOnMeshUVAtlasMulti(
	UObject* WorldContextObject,
	TArray<FRenderMaterialOnMeshPrimitive> Components,
	UMaterialInterface* Material, 
	UTextureRenderTarget2D* BaseColor, 
	UTextureRenderTarget2D* Emissive,
	UTextureRenderTarget2D* NormalMap,
	const FBox2D& UVRegion,
	bool bClearRenderTargets
)
{
	return RenderMaterialOnMeshUVAtlasMulti(WorldContextObject, MakeArrayView(Components), Material, BaseColor, Emissive, NormalMap, FRenderMaterialOnMeshViewConfiguration(), bClearRenderTargets);
}

bool UMeshPainterFunctionLibrary::RenderMaterialOnMeshUVAtlasMulti(
	UObject* WorldContextObject,
	TArrayView<FRenderMaterialOnMeshPrimitive> Components,
	UMaterialInterface* Material, 
	UTextureRenderTarget2D* BaseColor, 
	UTextureRenderTarget2D* Emissive,
	UTextureRenderTarget2D* NormalMap,
	const FRenderMaterialOnMeshViewConfiguration& ViewPointConfiguration,
	bool bClearRenderTargets
)
{
	// Must execute on the main thread
	check(IsInGameThread());

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (Components.IsEmpty() || !IsValid(World))
		return false;

	FMeshPaintRenderTargets Targets;
	Targets.SetRenderTarget(BaseColor, FMeshPaintRenderTargets::RT_BaseColor);
	Targets.SetRenderTarget(Emissive, FMeshPaintRenderTargets::RT_Emissive);
	Targets.SetRenderTarget(NormalMap, FMeshPaintRenderTargets::RT_NormalMap);
	if (!Targets.IsValidForRendering()) return false;

	const FIntPoint TargetSize = Targets.GetPrimaryRenderTarget()->GetSizeXY();

	FSceneViewProjectionData ViewInitOptions;
	ViewInitOptions.SetViewRectangle(FIntRect(0, 0, TargetSize.X, TargetSize.Y));
	ViewInitOptions.ViewOrigin = ViewPointConfiguration.ViewOrigin;
	ViewInitOptions.ViewRotationMatrix = ViewPointConfiguration.ViewRotationMatrix;
	ViewInitOptions.ProjectionMatrix = ViewPointConfiguration.ProjectionMatrix;

	FMeshPaintRenderParameters Params;
	Params.bClearTargets = bClearRenderTargets;
	Params.Scene = World->Scene;
	Params.MaterialOverride = Material ? Material->GetRenderProxy() : nullptr;
	Params.ViewProjection = ViewInitOptions;
	
	for (const FRenderMaterialOnMeshPrimitive& Prim : Components)
	{
		if (!IsValid(Prim.MeshComponent) || !Prim.MeshComponent->SceneProxy) continue;
		FMeshPaintProxyRenderParameters Param;
		Param.PrimitiveProxy = Prim.MeshComponent->SceneProxy;
		Param.TargetLOD = Prim.DesiredLOD;
		Param.UVRegion = Prim.UVRegion;
		Params.PrimitivesToRender.Add(Param);
	}

	if (Params.PrimitivesToRender.IsEmpty())
		return false;

	ENQUEUE_RENDER_COMMAND(RenderMaterialOnMeshUVLayoutCommand)(
	[=](FRHICommandListImmediate& RHICmdList)
	{
		Targets.FlushDeferredResourceUpdate(RHICmdList);
		MeshPaintRender::AddMeshPaintPass(RHICmdList, Targets, Params);
	});

	if (BaseColor) BaseColor->UpdateResourceImmediate(false);
	if (Emissive) Emissive->UpdateResourceImmediate(false);
	if (NormalMap) NormalMap->UpdateResourceImmediate(false);
	return true;
}
