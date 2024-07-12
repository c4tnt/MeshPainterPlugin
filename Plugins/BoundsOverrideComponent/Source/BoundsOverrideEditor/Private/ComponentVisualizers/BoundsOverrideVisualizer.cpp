#include "ComponentVisualizers/BoundsOverrideVisualizer.h"
#include "Components/BoundsOverrideComponent.h"
#include "SceneManagement.h"

typedef decltype(FBoxSphereBounds::BoxExtent) FBoxExtentType;
const FBoxExtentType PrivateCubeEdgeList[] = {
	FBoxExtentType(-1.0, -1.0, -1.0),		FBoxExtentType(1.0, -1.0, -1.0),
	FBoxExtentType(-1.0, -1.0, -1.0),		FBoxExtentType(-1.0, 1.0, -1.0),
	FBoxExtentType(-1.0, -1.0, -1.0),		FBoxExtentType(-1.0, -1.0, 1.0),
	FBoxExtentType(1.0, 1.0, 1.0),			FBoxExtentType(-1.0, 1.0, 1.0),
	FBoxExtentType(1.0, 1.0, 1.0),			FBoxExtentType(1.0, -1.0, 1.0),
	FBoxExtentType(1.0, 1.0, 1.0),			FBoxExtentType(1.0, 1.0, -1.0),
	FBoxExtentType(1.0, 1.0, -1.0),			FBoxExtentType(-1.0, 1.0, -1.0),
	FBoxExtentType(1.0, 1.0, -1.0),			FBoxExtentType(1.0, -1.0, -1.0),
	FBoxExtentType(1.0, -1.0, 1.0),			FBoxExtentType(-1.0, -1.0, 1.0),
	FBoxExtentType(1.0, -1.0, 1.0),			FBoxExtentType(1.0, -1.0, -1.0),
	FBoxExtentType(-1.0, 1.0, 1.0),			FBoxExtentType(-1.0, -1.0, 1.0),
	FBoxExtentType(-1.0, 1.0, 1.0),			FBoxExtentType(-1.0, 1.0, -1.0)
};
constexpr int32 EdgeCount = UE_ARRAY_COUNT(PrivateCubeEdgeList) >> 1;

void FBoundsOverrideComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (const UBoundsOverrideComponent* ComponentToVisualize = Cast<UBoundsOverrideComponent>(Component))
	{
		const double DashSizeRespectToView = FMath::Sqrt(View->Deproject(FPlane(3.1622, 3.1622, 1.0, 1.0)).Length());
		float SelectionLevel = 0.6f;
		if (Component->IsSelected())
		{
			SelectionLevel = 0.0f;
		}
		else if (Component->IsOwnerSelected())
		{
			SelectionLevel = 0.3f;
		}

		const FTransform ComponentToWorld = ComponentToVisualize->GetComponentTransform();
		const FLinearColor MainBoxColor = ComponentToVisualize->IsOverridingBounds() ? FLinearColor::Red : FLinearColor::Green;
		const FBoxSphereBounds ComputedBounds = ComponentToVisualize->CalcBounds(ComponentToWorld);
		for (int32 EdgeIndex = 0; EdgeIndex < EdgeCount; ++EdgeIndex)
		{
			const FBoxExtentType Start = ComputedBounds.Origin + ComputedBounds.BoxExtent * PrivateCubeEdgeList[EdgeIndex << 1];
			const FBoxExtentType End = ComputedBounds.Origin + ComputedBounds.BoxExtent * PrivateCubeEdgeList[(EdgeIndex << 1) + 1];
			DrawDashedLine(PDI, Start, End, MainBoxColor.Desaturate(SelectionLevel), DashSizeRespectToView, SDPG_World);
		}

		const FBoxSphereBounds EffectiveBounds = ComponentToVisualize->Bounds;
		if (!EffectiveBounds.GetBox().Equals(ComputedBounds.GetBox()))
		{
			for (int32 EdgeIndex = 0; EdgeIndex < EdgeCount; ++EdgeIndex)
			{
				const FBoxExtentType Start = EffectiveBounds.Origin + EffectiveBounds.BoxExtent * PrivateCubeEdgeList[EdgeIndex << 1];
				const FBoxExtentType End = EffectiveBounds.Origin + EffectiveBounds.BoxExtent * PrivateCubeEdgeList[(EdgeIndex << 1) + 1];
				DrawDashedLine(PDI, Start, End, FLinearColor::Yellow.Desaturate(SelectionLevel), DashSizeRespectToView, SDPG_World);
			}
		}
	}
}
