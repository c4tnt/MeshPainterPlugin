#pragma once

#include "CoreMinimal.h"
#include "Engine\MeshMerging.h"
#include "ExtendedMeshMergingSettings.generated.h"

class UWrappedMeshBuilder;

/** Mesh instance-replacement settings */
USTRUCT(Blueprintable)
struct FWrappedMeshInstancingSettings
{
	GENERATED_BODY()

	FWrappedMeshInstancingSettings();

	void InitializeWithinUObject(const FObjectInitializer& ObjectInitializer, UObject* Outer);

	/** The actor class to attach new instance static mesh components to */
	UPROPERTY(BlueprintReadWrite, Instanced, EditAnywhere, NoClear, Category="Instancing")
	UWrappedMeshBuilder* ActorBuilderInstance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, NoClear, Category="Instancing")
	FVector AnchorPoint;

	/** The number of static mesh instances needed before a mesh is replaced with an instanced version */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Instancing", meta=(ClampMin=1))
	int32 InstanceReplacementThreshold;

	/**
	* Whether to skip the conversion to an instanced static mesh for meshes with vertex colors.
	* Instanced static meshes do not support vertex colors per-instance, so conversion will lose
	* this data.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Instancing")
	bool bSkipMeshesWithVertexColors;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Instancing")
	bool bGatherNonMeshComponents;

	/**
	* Whether split up instanced static mesh components based on their intersection with HLOD volumes
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Instancing", meta=(DisplayName="Use HLOD Volumes"))
	bool bUseHLODVolumes;

	/**
	* Whether to use the Instanced Static Mesh Compoment or the Hierarchical Instanced Static Mesh Compoment
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Instancing", meta = (DisplayName = "Select the type of Instanced Component", DisallowedClasses = "/Script/Foliage.FoliageInstancedStaticMeshComponent"))
	TSubclassOf<UInstancedStaticMeshComponent> ISMComponentToUse;
};