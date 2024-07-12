#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"

struct FMeshPaintRenderTargets
{
	enum RTType	{ RT_BaseColor, RT_Emissive, RT_NormalMap };
	
	FTextureRenderTargetResource* BaseColor;
	FTextureRenderTargetResource* Emissive;
	FTextureRenderTargetResource* NormalMap;
	RTType PrimaryRendertTargetForMRTViews;

	FMeshPaintRenderTargets() : BaseColor(nullptr), Emissive(nullptr), NormalMap(nullptr), PrimaryRendertTargetForMRTViews(RTType::RT_BaseColor) {}

	bool SetRenderTarget(UTextureRenderTarget2D* RenderTarget, RTType Type)
	{
		if (!IsValid(RenderTarget)) return false;
		if (!RenderTarget->GetResource()) return false;
		FTextureRenderTargetResource* Result = RenderTarget->GameThread_GetRenderTargetResource();
		switch(Type)
		{
		case RTType::RT_BaseColor: BaseColor = Result; break;
		case RTType::RT_Emissive: Emissive = Result; break;
		case RTType::RT_NormalMap: NormalMap = Result; break;
		}
		if (Result != nullptr)
		{
			FTextureRenderTargetResource* PreviousPrimaryRT = GetPrimaryRenderTarget();
			if (PreviousPrimaryRT == nullptr || (PreviousPrimaryRT->GetSizeX() <= Result->GetSizeX() && PreviousPrimaryRT->GetSizeY() <= Result->GetSizeY()))
			{
				PrimaryRendertTargetForMRTViews = Type;
			}
			return true;
		}
		return false;
	}

	bool IsValidForRendering() const { return BaseColor != nullptr || Emissive != nullptr || NormalMap != nullptr; }

	void FlushDeferredResourceUpdate(FRHICommandListImmediate& RHICmdList) const
	{
		if (BaseColor) BaseColor->FlushDeferredResourceUpdate(RHICmdList);
		if (Emissive) Emissive->FlushDeferredResourceUpdate(RHICmdList);
		if (NormalMap) NormalMap->FlushDeferredResourceUpdate(RHICmdList);
	}

	FTextureRenderTargetResource* GetPrimaryRenderTarget() const
	{
		return GetRenderTargetOfType(PrimaryRendertTargetForMRTViews);
	}

	bool HasTargetOfType(RTType Type) const
	{
		switch(Type)
		{
		case RTType::RT_BaseColor: return BaseColor != nullptr;
		case RTType::RT_Emissive: return Emissive != nullptr;
		case RTType::RT_NormalMap: return NormalMap != nullptr;
		}
		return false;
	}

	FTextureRenderTargetResource* GetRenderTargetOfType(RTType Type) const
	{
		switch(Type)
		{
		case RTType::RT_BaseColor: return BaseColor;
		case RTType::RT_Emissive: return Emissive;
		case RTType::RT_NormalMap: return NormalMap;
		}
		return nullptr;
	}
};

struct FMeshPaintProxyRenderParameters
{
	FMeshPaintProxyRenderParameters() : PrimitiveProxy(nullptr), TargetLOD(0), UVRegion(FVector2D::Zero(), FVector2D::One()) {}

	/** Primitive scene proxy */
	FPrimitiveSceneProxy* PrimitiveProxy;

	/** Which LOD we want to render */
	int32 TargetLOD;

	/** Where on the screen we want to render this primitive (for atlasing) */
	FBox2D UVRegion;
};

struct FMeshPaintRenderParameters
{
	/** A list of primitive scene proxies to render */
	TArray<FMeshPaintProxyRenderParameters> PrimitivesToRender;

	/** Scene to use */
	FSceneInterface* Scene;
	
	/** View configuration except ViewFamily to use while rendering */
	FSceneViewProjectionData ViewProjection;

	/** Which material will be used to draw primitives. Will use primitive's materials when not specified */
	const FMaterialRenderProxy* MaterialOverride;
	
	bool bClearTargets;
};

namespace MeshPaintRender
{
	MESHPAINTERSHADERCORE_API bool AddMeshPaintPass(FRHICommandListImmediate& RHICmdList, const FMeshPaintRenderTargets& InRenderTargets, const FMeshPaintRenderParameters& Parameters);
	MESHPAINTERSHADERCORE_API bool AddMeshPaintPass(FRDGBuilder& GraphBuilder, const FMeshPaintRenderTargets& InRenderTargets, const FMeshPaintRenderParameters& Parameters);
}
