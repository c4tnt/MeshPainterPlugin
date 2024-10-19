#pragma once

#include "CoreMinimal.h"
#include "Engine/MeshMerging.h"
#include "Actors/BoundsWrapperActor.h"
#include "Components/BoundsOverrideComponent.h"
#include "WrappedMeshBuilder.generated.h"

class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct FWrappedMeshBuilderContext
{
	GENERATED_BODY()

public:	
	AActor* SpawnWrapperActor(UWorld* World, const FTransform& Pivot, const FActorSpawnParameters& Params) const;

	UPROPERTY(BlueprintReadOnly)
	UClass* ActorClass;

	UPROPERTY(BlueprintReadOnly)
	FBox MergedBounds;

	UPROPERTY(BlueprintReadOnly)
	AActor* WrapperActor;
};

UCLASS(Blueprintable, Abstract, EditInlineNew)
class UWrappedMeshBuilder : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Instancing")
	TSubclassOf<AActor> GetActorClassToUse() const;
	virtual TSubclassOf<AActor> GetActorClassToUse_Implementation() const { return nullptr; };

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Instancing")
	bool ShouldHarvest(USceneComponent* StaticMeshComponent) const;
	virtual bool ShouldHarvest_Implementation(USceneComponent* StaticMeshComponent) const { return true; }
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Instancing")
	void InitializeActor(AActor* SpawnedActor, UPARAM(Ref) FWrappedMeshBuilderContext& Context) const;
	virtual void InitializeActor_Implementation(AActor* SpawnedActor, FWrappedMeshBuilderContext& Context) const { }

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Instancing")
	void InitializeComponent(USceneComponent* Component, AActor* SpawnedActor, UPARAM(Ref) FWrappedMeshBuilderContext& Context) const;
	virtual void InitializeComponent_Implementation(USceneComponent* Component, AActor* SpawnedActor, FWrappedMeshBuilderContext& Context) const { }

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Instancing")
	void FinalizeActor(AActor* SpawnedActor, UPARAM(Ref) FWrappedMeshBuilderContext& Context) const;
	virtual void FinalizeActor_Implementation(AActor* SpawnedActor, FWrappedMeshBuilderContext& Context) const { }

	virtual bool PrepareBuilderContext(FWrappedMeshBuilderContext& Context, FText* ErrorMessageOut = nullptr) const;
};

UCLASS(Blueprintable)
class UWrappedMeshBuilderDefault : public UWrappedMeshBuilder
{
	GENERATED_BODY()

public:
	UWrappedMeshBuilderDefault();
	virtual TSubclassOf<AActor> GetActorClassToUse_Implementation() const override { return ActorClass; };
	virtual void InitializeActor_Implementation(AActor* SpawnedActor, FWrappedMeshBuilderContext& Context) const;
	virtual void InitializeComponent_Implementation(USceneComponent* Component, AActor* SpawnedActor, FWrappedMeshBuilderContext& Context) const;

protected:
	/** The actor class to attach new instance static mesh components to */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, NoClear, Category="Instancing")
	TSubclassOf<ABoundsWrapperActor> ActorClass;

	/** Which space to use in order to override bounds */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Instancing")
	EBoundsOverrideSpace BoundsSpace;

	/** Controls whether the primitive component should cast a shadow or not. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Component Settings")
	uint8 CastShadow:1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Component Settings", AdvancedDisplay, meta=(EditCondition="CastShadow", DisplayName = "Contact Shadow"))
	uint8 bCastContactShadow:1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Component Settings")
	uint8 UseAsOccluder : 1;
};