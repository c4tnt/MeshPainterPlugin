// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/BoundsWrapperActor.h"
#include "Components/BoundsOverrideComponent.h"
#include "Components/BillboardComponent.h"

// Sets default values
ABoundsWrapperActor::ABoundsWrapperActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	BoundsComponent = CreateDefaultSubobject<UBoundsOverrideComponent>(TEXT("BoundsComponent"));
	BoundsComponent->SetMobility(EComponentMobility::Static);
	SetRootComponent(BoundsComponent);

#if WITH_EDITOR
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTextureObject;
		FName                                                 ID_BoundsGroup;
		FText                                                 NAME_BoundsGroup;

		FConstructorStatics()
			: SpriteTextureObject(TEXT("/Engine/EditorResources/S_Actor"))
			, ID_BoundsGroup(TEXT("Bounds Group"))
			, NAME_BoundsGroup(NSLOCTEXT("SpriteCategory", "BoundsGroup", "Bounds Override Group"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	if (!IsRunningCommandlet())
	{
		SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Editor Icon"));
		if (SpriteComponent)
		{
			SpriteComponent->bIsEditorOnly = true;
			SpriteComponent->Sprite = ConstructorStatics.SpriteTextureObject.Get();
			SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_BoundsGroup;
			SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_BoundsGroup;
			SpriteComponent->Mobility = EComponentMobility::Static;
			SpriteComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
			SpriteComponent->SetupAttachment(BoundsComponent);
		}
	}
#endif
}
