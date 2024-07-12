// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoundsWrapperActor.generated.h"

class UBoundsOverrideComponent;
class UBillboardComponent;

UCLASS(Blueprintable, BlueprintType)
class BOUNDSOVERRIDERUNTIME_API ABoundsWrapperActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABoundsWrapperActor();

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="Components")
	UBoundsOverrideComponent* BoundsComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="Components")
	UBillboardComponent* SpriteComponent;
#endif
};
