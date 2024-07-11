// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "CustomMeshWidget.generated.h"

class USlateVectorArtData;
class SCustomMeshWidget;

UCLASS()
class MESHWIDGETRUNTIME_API UCustomMeshWidget : public UWidget
{
	GENERATED_BODY()

public:
	UCustomMeshWidget(const FObjectInitializer& ObjectInitializer);

	/** Mesh to draw */
	UPROPERTY(EditAnywhere, Category=Appearance)
	TObjectPtr<UStaticMesh> Mesh;

	/** Material for this mesh */
	UPROPERTY(EditAnywhere, Category=Appearance)
	TObjectPtr<UMaterialInterface> Material;

	/** Mesh local transform */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FTransform MeshTransform;

	/** Enable instanced draws */
	UPROPERTY(EditAnywhere, Category=Appearance)
	bool bEnableInstancing;

	/** Per instance transform */
	UPROPERTY(EditAnywhere, Category=Appearance, AdvancedDisplay, meta=(EditCondition="bEnableInstancing"))
	TArray<FTransform> InstanceTransforms;

	UFUNCTION(BlueprintCallable)
	void SetStaticMesh(UStaticMesh* InMesh);

	UFUNCTION(BlueprintCallable)
	UStaticMesh* GetStaticMesh() const;

	UFUNCTION(BlueprintCallable)
	void SetMaterial(UMaterialInterface* InMaterial);

	UFUNCTION(BlueprintCallable)
	UMaterialInterface* GetMaterial();

	UFUNCTION(BlueprintCallable)
	void SetAutoMaterial();

	UFUNCTION(BlueprintCallable)
	bool IsInstanced() const;

	UFUNCTION(BlueprintCallable)
	bool IsInstanceDataDirty() const;

	UFUNCTION(BlueprintCallable)
	int32 GetNumInstances() const;

	UFUNCTION(BlueprintCallable)
	bool UpdateInstanceTransform(int32 InstanceIndex, const FTransform& Transform);

	// Compute mesh bounds in widget space
	UFUNCTION(BlueprintCallable)
	FBox GetLocalSpaceBounds() const;

	// Converts assigned brush material to a dynamic instance when possible
	UFUNCTION(BlueprintCallable)
	UMaterialInstanceDynamic* GetDynamicMaterialInstance(UObject* Outer, bool bCreateSingleUser = false);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	// Begin UWidget Interface
	virtual const FText GetPaletteCategory() override;
	// End UWidget Interface
#endif

protected:
	// Native Slate Widget
	TSharedPtr<SCustomMeshWidget> MeshWidget;

	// UWidget Interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
};