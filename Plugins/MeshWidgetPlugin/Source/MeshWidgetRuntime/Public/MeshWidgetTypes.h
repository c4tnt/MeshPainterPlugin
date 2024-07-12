#pragma once

#include "CoreMinimal.h"

enum ECustomWidgetInstancingType
{
	NoInstancing,
	EnableInstancing,
	EnableAndCreateDefaultInstance
};

struct FCustomMeshWidgetInstanceData
{
	FQuat Rot;
	FVector Origin;
	FVector Scale;

	FORCEINLINE FMatrix GetInstanceTransform() const { return FScaleRotationTranslationMatrix(Scale, Rot.Rotator(), Origin); };
	FORCEINLINE void SetTransform(const FTransform& Tr)
	{ 
		Origin = Tr.GetLocation();
		Rot = Tr.GetRotation();
		Scale = Tr.GetScale3D();
	};
};
