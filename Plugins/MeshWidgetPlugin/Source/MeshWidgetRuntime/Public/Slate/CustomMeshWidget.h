#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "Textures/SlateShaderResource.h"
#include "Rendering/RenderingCommon.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SLeafWidget.h"
#include "MeshWidgetTypes.h"

class FPaintArgs;
class FSlateWindowElementList;
class UMaterialInstanceDynamic;
class USlateVectorArtData;
class FCustomMeshSlateElement;
struct FSlateBrush;

struct FInstanceUpdateBatch
{
	int32 StartIndex;
	int32 Length;
};

class MESHWIDGETRUNTIME_API SCustomMeshWidget : public SLeafWidget, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SCustomMeshWidget) 
		: _Mesh(nullptr)
		, _Instancing(ECustomWidgetInstancingType::NoInstancing)
		, _MeshTransform(FMatrix::Identity)
		, _Animated(false) 
		{}
		SLATE_ARGUMENT(UStaticMesh*, Mesh)
		SLATE_ARGUMENT(ECustomWidgetInstancingType, Instancing)
		SLATE_ARGUMENT(FMatrix, MeshTransform)
		SLATE_ATTRIBUTE(bool, Animated)
	SLATE_END_ARGS()

	~SCustomMeshWidget() { ReleaseRenderResources(); }

	void Construct(const FArguments& Args);

	void SetStaticMesh(UStaticMesh* Mesh);
	UStaticMesh* GetStaticMesh() const { return Mesh.Get(); }
	UMaterialInterface* GetMaterial() const;

	void SetMeshTransform(const FTransform& NewTransform);
	void SetMeshTransform(const FMatrix& NewTransform);
	bool IsSameMeshTransform(const FTransform& NewTransform, double Tolerance = KINDA_SMALL_NUMBER) const { return MeshTransform.Equals(NewTransform.ToMatrixWithScale(), Tolerance); }
	bool IsSameMeshTransform(const FMatrix& NewTransform, double Tolerance = KINDA_SMALL_NUMBER) const { return MeshTransform.Equals(NewTransform, Tolerance); }
	FMatrix GetMeshTransform() const { return MeshTransform; }

	void SetMaterial(UMaterialInterface* Material);
	void SetAutoMaterial();
	FBox GetLocalSpaceBounds() const { return CachedBounds; }

	bool IsValidForRendering() const;
	bool IsInstanced() const { return bIsInstanced; }
	bool IsInstanceDataDirty() const { return bIsInstanced && InstanceInfoPendingUpdate.Num(); }
	int32 GetNumInstances() const { return bIsInstanced ? InstanceData.Num() : 1; }
	void SetInstancingMode(ECustomWidgetInstancingType NewMode);

	bool UpdateInstance(int32 InstanceIndex, const FCustomMeshWidgetInstanceData& InstanceData);
	bool UpdateInstanceTransform(int32 InstanceIndex, const FTransform& Transform);

	/** Compute how widget is changed and invalidate it **/
	void InvalidateWidget(bool bConsiderRedraw, bool bConsiderResize);

	// Converts assigned brush material to a dynamic instance when possible
	UMaterialInstanceDynamic* GetDynamicMaterialInstance(UObject* Outer, bool bCreateSingleUser = false);

protected:
	// BEGIN SLeafWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	// END SLeafWidget interface

	// ~ FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;
	// ~ FGCObject

	/** Create new renderer object **/
	void CreateRenderResources();

	/** Release all mesh rendering related resources **/
	void ReleaseRenderResources();

	// Compute mesh bounds in widget space
	bool InvalidateBounds();

	/** What mesh we are using to draw. */
	TObjectPtr<UStaticMesh> Mesh;

	/** What mesh we are using to draw. */
	TObjectPtr<UMaterialInterface> Material;

	/** Mesh bounds that are used to determine desired size. */
	FBox CachedBounds;
	
	/** Whole mesh set transform. */
	FMatrix MeshTransform;

	/** Mesh instances to render (used when instancing is active) **/
	TArray<FCustomMeshWidgetInstanceData> InstanceData;

	/** Contains an information which instances are to be updated */
	TArray<FInstanceUpdateBatch> InstanceInfoPendingUpdate;

	/** Widget rasterizer state */
	TSharedPtr<FCustomMeshSlateElement, ESPMode::ThreadSafe> MeshRenderer;

	/** Whether this widget performs instanced draws or not **/
	bool bIsInstanced;

	/** Animate widget over the time **/
	TAttribute<bool> IsAnimatedAttribute;
};
