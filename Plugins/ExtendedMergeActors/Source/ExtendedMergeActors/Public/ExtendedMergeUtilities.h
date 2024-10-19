#pragma once

#include "CoreMinimal.h"

class UPrimitiveComponent;
class UWorld;
class ULevel;
struct FWrappedMeshInstancingSettings;

namespace FMeshMergeUtilitiesEx
{
	void WrapComponentsToInstances(const TArray<USceneComponent*>& ComponentsToMerge, UWorld* World, ULevel* Level, const FWrappedMeshInstancingSettings& InSettings, bool bActuallyMerge /*= true*/, bool bReplaceSourceActors /* = false */, FText* OutResultsText /*= nullptr*/);
};
