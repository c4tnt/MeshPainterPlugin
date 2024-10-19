#include "WrappedMeshBuilder.h"

#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/BillboardComponent.h"

#define LOCTEXT_NAMESPACE "WrappedMeshBuilder"

AActor* FWrappedMeshBuilderContext::SpawnWrapperActor(UWorld* World, const FTransform& Pivot, const FActorSpawnParameters& Params) const
{
	check(ActorClass);
	return World->SpawnActor<AActor>(ActorClass, Pivot, Params);
}

bool UWrappedMeshBuilder::PrepareBuilderContext(FWrappedMeshBuilderContext& Context, FText* ErrorMessageOut) const
{
	Context.ActorClass = GetActorClassToUse();
	if (!Context.ActorClass || !Context.ActorClass->IsChildOf(AActor::StaticClass()))
	{
		if (ErrorMessageOut)
		{
			*ErrorMessageOut = LOCTEXT("WrappedMeshBuilder_BadActorSpawnclass", "Bad actor class is provided to create merged actors.");
		}
		return false;
	}

	return true;
}

UWrappedMeshBuilderDefault::UWrappedMeshBuilderDefault()
{
	ActorClass = ABoundsWrapperActor::StaticClass();
	BoundsSpace = EBoundsOverrideSpace::Local;
}

void UWrappedMeshBuilderDefault::InitializeActor_Implementation(AActor* SpawnedActor, FWrappedMeshBuilderContext& Context) const
{
	if (UBoundsOverrideComponent* BoundsComponent = Cast<UBoundsOverrideComponent>(SpawnedActor->GetRootComponent()))
	{
		BoundsComponent->SetBoundsSpace(BoundsSpace, false);
		BoundsComponent->SetBoundsOverriden(EBoundsOverrideSpace::World, Context.MergedBounds, true);
	}
	if (UBillboardComponent* EditorSpriteComponent = SpawnedActor->GetComponentByClass<UBillboardComponent>())
	{
		EditorSpriteComponent->SetRelativeLocation(FVector(0.0f, 0.0f, Context.MergedBounds.GetSize().Z));
	}
}

void UWrappedMeshBuilderDefault::InitializeComponent_Implementation(USceneComponent* Component, AActor* SpawnedActor, FWrappedMeshBuilderContext& Context) const
{
	Component->bUseAttachParentBound = true;
	if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
	{
		PrimitiveComponent->CastShadow = CastShadow;
		PrimitiveComponent->bCastContactShadow = bCastContactShadow;
		PrimitiveComponent->bUseAsOccluder = UseAsOccluder;
	}
}

#undef LOCTEXT_NAMESPACE