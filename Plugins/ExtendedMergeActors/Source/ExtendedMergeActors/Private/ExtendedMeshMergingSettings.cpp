#include "ExtendedMeshMergingSettings.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/BrushComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "WrappedMeshBuilder.h"

#define LOCTEXT_NAMESPACE "FExtendedMeshInstancingTool"

FWrappedMeshInstancingSettings::FWrappedMeshInstancingSettings()
	: ActorBuilderInstance(nullptr)
	, AnchorPoint(0.5f, 0.5f, 0.0f)
	, InstanceReplacementThreshold(2)
	, bSkipMeshesWithVertexColors(true)
	, bGatherNonMeshComponents(true)
	, bUseHLODVolumes(true)
	, ISMComponentToUse(UInstancedStaticMeshComponent::StaticClass())
{
}

void FWrappedMeshInstancingSettings::InitializeWithinUObject(const FObjectInitializer& ObjectInitializer, UObject* Outer)
{
	ActorBuilderInstance = ObjectInitializer.CreateDefaultSubobject<UWrappedMeshBuilderDefault>(Outer, TEXT("DefaultBuilder"));
}

#undef LOCTEXT_NAMESPACE
