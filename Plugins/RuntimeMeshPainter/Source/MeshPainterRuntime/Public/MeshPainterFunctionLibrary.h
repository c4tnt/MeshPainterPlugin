// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "MeshPainterFunctionLibrary.generated.h"

USTRUCT(BlueprintType)
struct FRenderMaterialOnMeshPrimitive
{
	GENERATED_BODY()

	FRenderMaterialOnMeshPrimitive() : MeshComponent(nullptr), DesiredLOD(0), DesiredUV(0), UVRegion(FBox2D(FVector2D::Zero(), FVector2D::One())) {};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UPrimitiveComponent* MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DesiredLOD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DesiredUV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBox2D UVRegion;
};

USTRUCT(BlueprintType)
struct FRenderMaterialOnMeshViewConfiguration
{
	GENERATED_BODY()

	FRenderMaterialOnMeshViewConfiguration() : ViewOrigin(FVector::ZeroVector), ViewRotationMatrix(FMatrix::Identity), ProjectionMatrix(FOrthoMatrix(512, 512, 1.0f, 0.0f)) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ViewOrigin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMatrix ViewRotationMatrix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMatrix ProjectionMatrix;
};

/**
 * 
 */
UCLASS()
class RUNTIMEMESHPAINTER_API UMeshPainterFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static bool RenderMaterialOnMeshUVLayout(
		UObject* WorldContextObject,
		UPrimitiveComponent* MeshComponent, 
		UMaterialInterface* Material, 
		UTextureRenderTarget2D* BaseColor, 
		UTextureRenderTarget2D* Emissive,
		UTextureRenderTarget2D* NormalMap,
		int32 LOD, 
		bool bClearRenderTargets);

	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static bool RenderMaterialOnMeshUVAtlas(
		UObject* WorldContextObject,
		UPrimitiveComponent* MeshComponent, 
		UMaterialInterface* Material, 
		UTextureRenderTarget2D* BaseColor, 
		UTextureRenderTarget2D* Emissive,
		UTextureRenderTarget2D* NormalMap,
		int32 LOD, 
		const FBox2D& UVRegion,
		bool bClearRenderTargets
	);

	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static bool RenderMaterialOnMeshUVAtlasMulti(
		UObject* WorldContextObject,
		TArray<FRenderMaterialOnMeshPrimitive> Components,
		UMaterialInterface* Material, 
		UTextureRenderTarget2D* BaseColor, 
		UTextureRenderTarget2D* Emissive,
		UTextureRenderTarget2D* NormalMap,
		const FBox2D& UVRegion,
		bool bClearRenderTargets
	);

	static bool RenderMaterialOnMeshUVAtlasMulti(
		UObject* WorldContextObject,
		TArrayView<FRenderMaterialOnMeshPrimitive> Components,
		UMaterialInterface* Material, 
		UTextureRenderTarget2D* BaseColor, 
		UTextureRenderTarget2D* Emissive,
		UTextureRenderTarget2D* NormalMap,
		const FRenderMaterialOnMeshViewConfiguration& ViewPointConfiguration,
		bool bClearRenderTargets
	);
};
