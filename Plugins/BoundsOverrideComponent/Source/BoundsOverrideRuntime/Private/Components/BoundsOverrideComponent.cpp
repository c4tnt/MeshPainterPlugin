// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/BoundsOverrideComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ComponentUtils.h"

DECLARE_CYCLE_STAT(TEXT("BoundsOverrideComponent CalcBounds"), STAT_BoundsOverrideComponentCalcBounds, STATGROUP_Component);

// Sets default values for this component's properties
UBoundsOverrideComponent::UBoundsOverrideComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;

	bOverrideBounds = false;
	CapturedBoundsSpace = EBoundsOverrideSpace::Local;
	bCookBounds = false;
	bUpdateBoundsAtRuntime = false;
	bCaptureNonCollidingComponents = false;
	bCaptureEditorOnlyComponents = false;
	bComputeBoundsOnceForGame = false;
	BoundsOverride = FBox(EForceInit::ForceInit);
	BoundsExpansion = 0.0f;
}


// Called when the game starts
void UBoundsOverrideComponent::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void UBoundsOverrideComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UBoundsOverrideComponent::InitializeComponent()
{
	Super::InitializeComponent();
	/*if (!bOverrideBounds)
	{
		RecomputeChildBounds(false);
	}*/
}

FBox UBoundsOverrideComponent::CalcBoundsBox(const FTransform& LocalToWorld) const
{
	const FBox ComputedBounds = bOverrideBounds ? BoundsOverride.GetBox() : GatherChildBounds(CapturedBoundsSpace, false);
	return ComputedBounds.IsValid ?
		ResolveAndTransformBounds(CapturedBoundsSpace, EBoundsOverrideSpace::Local, LocalToWorld.ToMatrixWithScale(), ComputedBounds).ExpandBy(BoundsExpansion) : 
		ComputedBounds;
}

/** Calculate the bounds of the component. Default behavior is a bounding box/sphere of zero size. */
FBoxSphereBounds UBoundsOverrideComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FBox ComputedBounds = CalcBoundsBox(LocalToWorld);
	return ComputedBounds.IsValid ? ComputedBounds : Super::CalcBounds(LocalToWorld).ExpandBy(BoundsExpansion);
}

/** Update the Bounds of the component.*/
void UBoundsOverrideComponent::UpdateBounds()
{
	// Calculate new bounds
	const UWorld* const World = GetWorld();
	const bool bIsGameWorld = World && World->IsGameWorld();

	SCOPE_CYCLE_COUNTER(STAT_BoundsOverrideComponentCalcBounds);
	if (GetUpdateBoundsAtRuntime() || !bIsGameWorld)
	{
		Bounds = CalcBounds(GetComponentTransform());
		bComputedBoundsOnceForGame = false;
	}
	else if (!bComputeBoundsOnceForGame || !bComputedBoundsOnceForGame)
	{
		const FBox ComputedBounds = CalcBoundsBox(GetComponentTransform());
		if (ComputedBounds.IsValid)
		{
			// If box successfully computed accept it
			Bounds = ComputedBounds;
			bComputedBoundsOnceForGame = (bIsGameWorld || IsRunningCookCommandlet()) && bComputeBoundsOnceForGame;
		}
		else
		{
			// Otherwise set 
			Bounds = Super::CalcBounds(GetComponentTransform());
		}
	}
}

void GetBBPoints(TArray<FVector>& Points, const FBoxSphereBounds& LocalBounds, const FMatrix& BoundTransform)
{
	// Generate local points on edge of box and sphere intersection shell
	static const FVector PointsBuildTable[] = {FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 0.5f), FVector(0.0f, 0.0f, 1.0f), FVector(0.0f, 0.5f, 0.0f),
		FVector(0.0f, 0.5f, 0.5f), FVector(0.0f, 0.5f, 1.0f), FVector(0.0f, 1.0f, 0.0f), FVector(0.0f, 1.0f, 0.5f), FVector(0.0f, 1.0f, 1.0f),
		FVector(0.5f, 0.0f, 0.0f), FVector(0.5f, 0.0f, 0.5f), FVector(0.5f, 0.0f, 1.0f), FVector(0.5f, 0.5f, 0.0f), FVector(0.5f, 0.5f, 0.5f),
		FVector(0.5f, 0.5f, 1.0f), FVector(0.5f, 1.0f, 0.0f), FVector(0.5f, 1.0f, 0.5f), FVector(0.5f, 1.0f, 1.0f), FVector(1.0f, 0.0f, 0.0f),
		FVector(1.0f, 0.0f, 0.5f), FVector(1.0f, 0.0f, 1.0f), FVector(1.0f, 0.5f, 0.0f), FVector(1.0f, 0.5f, 0.5f), FVector(1.0f, 0.5f, 1.0f),
		FVector(1.0f, 1.0f, 0.0f), FVector(1.0f, 1.0f, 0.5f), FVector(1.0f, 1.0f, 1.0f)};

	const FVector Max = LocalBounds.GetBoxExtrema(1);
	const FVector Min = LocalBounds.GetBoxExtrema(0);

	const uint32 PointsCount = UE_ARRAY_COUNT(PointsBuildTable);
	Points.Reserve(PointsCount);
	for (const FVector& LerpVector : MakeArrayView(PointsBuildTable, PointsCount))
	{
		const FVector LocalBoxVector = FMath::Lerp(Min, Max, LerpVector);    // .GetClampedToMaxSize(LocalBounds.SphereRadius);
		Points.Add(BoundTransform.TransformPosition(LocalBoxVector));
	}
}

void GatherChildBounds_r(
	const USceneComponent* Self, 
	bool bForceRefreshBounds, 
	bool bCaptureNonCollidingComponents, 
	bool bCaptureEditorOnlyComponents,
	TArray<FVector>& PointsWS, 
	const FMatrix& ParentToAccum)
{
	TArray<USceneComponent*> Children;
	Self->GetChildrenComponents(false, Children);
	for (USceneComponent* ChildComponent : Children)
	{
		bool bShouldCapture = (bCaptureNonCollidingComponents || ChildComponent->IsCollisionEnabled()) && (bCaptureEditorOnlyComponents || !ChildComponent->IsEditorOnly());
		FMatrix ComponentToAccum = (ChildComponent->GetRelativeTransform().ToMatrixWithScale() * ParentToAccum);

		if (bShouldCapture)
		{
			if (bForceRefreshBounds)
			{
				ChildComponent->UpdateBounds();
			}

			if (UInstancedStaticMeshComponent* ISMOwner = Cast<UInstancedStaticMeshComponent>(ChildComponent))
			{
				if (UStaticMesh* InstancedMeshAsset = ISMOwner->GetStaticMesh())
				{
					const auto InstanceLocalBounds = InstancedMeshAsset->GetBoundingBox();
					for (int32 Index = 0; Index < ISMOwner->PerInstanceSMData.Num(); Index++)
					{
						const FMatrix InstanceBoundSpace = ISMOwner->PerInstanceSMData[Index].Transform * ComponentToAccum;
						const FBoxSphereBounds LocalBounds = InstancedMeshAsset->GetBounds();
						GetBBPoints(PointsWS, LocalBounds, InstanceBoundSpace);
					}
				}
				else
				{
					// Failsafe path if mesh is unavailable
					for (int32 Index = 0; Index < ISMOwner->PerInstanceSMData.Num(); Index++)
					{
						const FMatrix InstanceBoundSpace = ISMOwner->PerInstanceSMData[Index].Transform * ComponentToAccum;
						PointsWS.Add(InstanceBoundSpace.TransformPosition(FVector::ZeroVector));
					}
				}
			}
			else
			{
				const FBoxSphereBounds LocalBounds = ChildComponent->CalcBounds(FTransform::Identity);
				GetBBPoints(PointsWS, LocalBounds, ComponentToAccum);
			}
		}
		GatherChildBounds_r(ChildComponent, bForceRefreshBounds, bCaptureNonCollidingComponents, bCaptureEditorOnlyComponents, PointsWS, ComponentToAccum);
	}
}



/** Compute children bounds of components */
FBox UBoundsOverrideComponent::GatherChildBounds(EBoundsOverrideSpace Space, bool bForceRefreshBounds) const
{
	TArray<FVector> Points;
	GatherChildBounds_r(this, bForceRefreshBounds, bCaptureNonCollidingComponents, bCaptureEditorOnlyComponents, Points, ComputeComponentToSpaceMatrix(Space));
	return Points.Num() ? FBox(Points) : FBox();
}

void UBoundsOverrideComponent::UseChildBounds()
{
	bOverrideBounds = true;
	BoundsOverride = GatherChildBounds(CapturedBoundsSpace, false);
}

bool UBoundsOverrideComponent::IsOverridingBounds() const
{
	return bOverrideBounds;
}

bool UBoundsOverrideComponent::GetUpdateBoundsAtRuntime() const
{
	return !bOverrideBounds && bUpdateBoundsAtRuntime && !bCookBounds;
}

void UBoundsOverrideComponent::SetUpdateBoundsAtRuntime(bool bShouldUpdate)
{
	bUpdateBoundsAtRuntime = !bCookBounds & bShouldUpdate;
}

void UBoundsOverrideComponent::SetBoundsOverriden(EBoundsOverrideSpace Space, const FBox& InBounds, bool bKeepNativeSpace)
{
	bOverrideBounds = true;
	if (bKeepNativeSpace)
	{
		BoundsOverride = ResolveBounds(Space, CapturedBoundsSpace, InBounds);
	}
	else
	{
		CapturedBoundsSpace = Space;
		BoundsOverride = InBounds;
	}
	bComputedBoundsOnceForGame = false;
	UpdateBounds();
}

void UBoundsOverrideComponent::SetBoundsAuto(EBoundsOverrideSpace Space)
{
	bOverrideBounds = false;
	CapturedBoundsSpace = Space;
	RecomputeChildBounds(false);
}

void UBoundsOverrideComponent::SetBoundsSpace(EBoundsOverrideSpace Space, bool bPreserveBounds)
{
	if (Space == CapturedBoundsSpace)
	{
		return;
	}
	else if (!bPreserveBounds)
	{
		CapturedBoundsSpace = Space;
	}
	else if (bOverrideBounds)
	{
		BoundsOverride = ResolveBounds(CapturedBoundsSpace, Space, BoundsOverride.GetBox());
		CapturedBoundsSpace = Space;
		bComputedBoundsOnceForGame = false;
		UpdateBounds();
	}
	else
	{
		CapturedBoundsSpace = Space;
		RecomputeChildBounds(false);
	}
}

FMatrix UBoundsOverrideComponent::ComputeComponentToSpaceMatrix(EBoundsOverrideSpace Space) const
{
	if (Space == EBoundsOverrideSpace::Local)
	{
		return FMatrix::Identity;
	}
	if (AActor* Owner = GetOwner())
	{
		if (const USceneComponent* RootComponent = Owner->GetRootComponent())
		{
			// Traverse up to the root component and collect transforms
			FMatrix CumulativeTransform = FMatrix::Identity;
			const USceneComponent* CurrentComponent = this;
			while (CurrentComponent && CurrentComponent != RootComponent)
			{
				CumulativeTransform = CumulativeTransform * CurrentComponent->GetRelativeTransform().ToMatrixWithScale();
				CurrentComponent = CurrentComponent->GetAttachParent();
			}
			// At this point we have actor space transform
			if (Space == EBoundsOverrideSpace::Actor)
			{
				return CumulativeTransform;
			}
			// As we've already tried all other options it must be world transform
			return CumulativeTransform * RootComponent->GetRelativeTransform().ToMatrixWithScale();
		}
	}
	// Both world and actor spaces are equal to the relative transform of the component if it isn't linked to somewhere
	return GetRelativeTransform().ToMatrixWithScale();
}

FMatrix UBoundsOverrideComponent::ComputeSpaceTransform(EBoundsOverrideSpace SourceSpace, EBoundsOverrideSpace TargetSpace) const
{
	return (SourceSpace == TargetSpace) ? FMatrix::Identity : (ComputeComponentToSpaceMatrix(SourceSpace).Inverse() * ComputeComponentToSpaceMatrix(TargetSpace));
}

FBox UBoundsOverrideComponent::ResolveBounds(EBoundsOverrideSpace SourceSpace, EBoundsOverrideSpace TargetSpace, const FBox& SourceBounds) const
{
	return (SourceSpace == TargetSpace) ? SourceBounds : SourceBounds.TransformBy(ComputeSpaceTransform(SourceSpace, TargetSpace));
}

FBox UBoundsOverrideComponent::ResolveAndTransformBounds(EBoundsOverrideSpace SourceSpace, EBoundsOverrideSpace TargetSpace, const FMatrix& Transform, const FBox& SourceBounds) const
{
	return SourceBounds.TransformBy((SourceSpace == TargetSpace) ? Transform : (ComputeSpaceTransform(SourceSpace, TargetSpace) * Transform));
}

FBox UBoundsOverrideComponent::GetExplicitChildBounds(EBoundsOverrideSpace TargetSpace) const
{
	return GatherChildBounds(TargetSpace, false);
}

bool UBoundsOverrideComponent::RecomputeChildBounds(bool bForceRefreshBounds)
{
	const UWorld* const World = GetWorld();
	const bool bIsGameWorld = World && World->IsGameWorld();
	FBox PreviousCachedBounds = Bounds.GetBox();
	Bounds = GatherChildBounds(CapturedBoundsSpace, bForceRefreshBounds);
	bComputedBoundsOnceForGame = (bIsGameWorld || IsRunningCookCommandlet()) && bComputeBoundsOnceForGame;
	return !Bounds.GetBox().Equals(PreviousCachedBounds);
}

#if WITH_EDITOR
void UBoundsOverrideComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
	if (PropertyThatChanged)
	{
		FName PropName = PropertyThatChanged->GetFName();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool UBoundsOverrideComponent::CanEditChange(const FProperty* Property) const
{
	return Super::CanEditChange(Property);
}
#endif

