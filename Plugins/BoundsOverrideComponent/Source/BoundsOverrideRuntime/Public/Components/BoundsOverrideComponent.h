// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "BoundsOverrideComponent.generated.h"

UENUM(BlueprintType)
enum class EBoundsOverrideSpace : uint8
{
	Local,
	World,
	Actor
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BOUNDSOVERRIDERUNTIME_API UBoundsOverrideComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBoundsOverrideComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Initialize after registration */
	virtual void InitializeComponent() override;

	/** Calculate the bounds of the component. Default behavior is a bounding box/sphere of zero size. */
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const final;

	/** Update the Bounds of the component.*/
	virtual void UpdateBounds();

	/** Update the Bounds of the component.*/
	virtual FBox GatherChildBounds(EBoundsOverrideSpace Space, bool bForceRefreshBounds) const;

	/** Save child native bounds as MinimalBoundingBox */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Default")
	void UseChildBounds();

	UFUNCTION(BlueprintCallable, Category = "Default")
	bool IsOverridingBounds() const;

	UFUNCTION(BlueprintCallable, Category = "Default")
	bool GetUpdateBoundsAtRuntime() const;

	UFUNCTION(BlueprintCallable, Category = "Default")
	void SetUpdateBoundsAtRuntime(bool bShouldUpdate);
	
	UFUNCTION(BlueprintCallable, Category = "Default")
	void SetBoundsOverriden(EBoundsOverrideSpace Space, const FBox& Bounds, bool bKeepNativeSpace = false);

	UFUNCTION(BlueprintCallable, Category = "Default")
	void SetBoundsAuto(EBoundsOverrideSpace Space);

	UFUNCTION(BlueprintCallable, Category = "Default")
	void SetBoundsSpace(EBoundsOverrideSpace Space, bool bPreserveBounds = true);

	UFUNCTION(BlueprintCallable, Category = "Default")
	FBox GetExplicitChildBounds(EBoundsOverrideSpace TargetSpace) const;

	UFUNCTION(BlueprintCallable, Category = "Default")
	bool RecomputeChildBounds(bool bForceRefreshBounds);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool CanEditChange(const FProperty* Property) const override;
#endif

protected:
	virtual FBox CalcBoundsBox(const FTransform& LocalToWorld) const;

	FMatrix ComputeComponentToSpaceMatrix(EBoundsOverrideSpace Space) const;
	FMatrix ComputeSpaceTransform(EBoundsOverrideSpace SourceSpace, EBoundsOverrideSpace TargetSpace) const;
	FBox ResolveBounds(EBoundsOverrideSpace SourceSpace, EBoundsOverrideSpace TargetSpace, const FBox& SourceBounds) const;
	FBox ResolveAndTransformBounds(EBoundsOverrideSpace SourceSpace, EBoundsOverrideSpace TargetSpace, const FMatrix& Transform, const FBox& SourceBounds) const;

private:

    UPROPERTY(EditAnywhere, Category = "Bounds Override")
	bool bOverrideBounds;

	UPROPERTY(EditAnywhere, Category = "Bounds Override")
	EBoundsOverrideSpace CapturedBoundsSpace;

	UPROPERTY(EditAnywhere, Category = "Bounds Override")
	uint32 bCookBounds : 1;

	UPROPERTY(EditAnywhere, Category = "Bounds Override", meta = (EditCondition = "!bCookBounds && !bOverrideBounds"))
	uint32 bUpdateBoundsAtRuntime : 1;

	UPROPERTY(EditAnywhere, Category = "Bounds Override")
	uint32 bCaptureNonCollidingComponents : 1;
	
	UPROPERTY(EditAnywhere, Category = "Bounds Override")
	uint32 bCaptureEditorOnlyComponents : 1;

    UPROPERTY(EditAnywhere, Category = "Bounds Override", meta = (EditCondition = "bOverrideBounds"))
	FBoxSphereBounds BoundsOverride;

	UPROPERTY(EditAnywhere, Category = "Bounds Capture")
	float BoundsExpansion;
};
