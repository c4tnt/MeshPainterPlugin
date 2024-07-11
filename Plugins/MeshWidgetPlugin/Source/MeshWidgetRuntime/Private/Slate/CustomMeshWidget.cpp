#include "Slate/CustomMeshWidget.h"
#include "SlateMaterialBrush.h"
#include "Render/CustomMeshElement.h"
#include "Fonts/FontMeasure.h"

void SCustomMeshWidget::Construct(const FArguments& Args)
{
	IsAnimatedAttribute = Args._Animated;
	SetStaticMesh(Args._Mesh);
	switch (Args._Instancing)
	{
	case ECustomWidgetInstancingType::EnableAndCreateDefaultInstance:
		// Emplaces default instance
		InstanceData.Emplace();
	case ECustomWidgetInstancingType::EnableInstancing:
		bIsInstanced = true;
		break;
	case ECustomWidgetInstancingType::NoInstancing:
	default:
		bIsInstanced = false;
	}

	// Set default transform
	MeshTransform = Args._MeshTransform;

	// Force bounds to be recomputed now
	InvalidateBounds();

	// Create resources if possible, or request for postponed update
	if (IsValidForRendering())
	{
		CreateRenderResources();
	}
}

void SCustomMeshWidget::SetStaticMesh(UStaticMesh* InMesh)
{
	if (Mesh == InMesh) return;
	Mesh = InMesh;
	ReleaseRenderResources();
	InvalidateWidget(true, true);
}

UMaterialInterface* SCustomMeshWidget::GetMaterial() const 
{ 
	if (Material) return Material.Get();
	if (!Mesh || Mesh->GetStaticMaterials().Num() < 1) return nullptr;
	return Mesh->GetStaticMaterials()[0].MaterialInterface;
}

void SCustomMeshWidget::SetMaterial(UMaterialInterface* InMaterial)
{
	if (Material == InMaterial) return;
	if (InMaterial != GetMaterial())
	{
		ReleaseRenderResources();
		InvalidateWidget(IsValidForRendering(), false);
	}
	Material = InMaterial;
}

void SCustomMeshWidget::SetAutoMaterial()
{
	SetMaterial(nullptr);
}

void SCustomMeshWidget::SetMeshTransform(const FTransform& NewTransform)
{
	MeshTransform = NewTransform.ToMatrixWithScale();
	InvalidateWidget(IsValidForRendering(), true);
}

void SCustomMeshWidget::SetMeshTransform(const FMatrix& NewTransform)
{
	MeshTransform = NewTransform;
	InvalidateWidget(IsValidForRendering(), true);
}

UMaterialInstanceDynamic* SCustomMeshWidget::GetDynamicMaterialInstance(UObject* Outer, bool bCreateSingleUser)
{
	if (!IsValid(Material)) return nullptr;
	if (Material->IsA<UMaterialInstanceDynamic>() && !bCreateSingleUser) return CastChecked<UMaterialInstanceDynamic>(Material);
	UMaterialInstanceDynamic* NewMID = UMaterialInstanceDynamic::Create(Material, Outer);
	SetMaterial(NewMID);
	return NewMID;
}

void SCustomMeshWidget::SetInstancingMode(ECustomWidgetInstancingType NewMode)
{
	bool bOldInstancing = bIsInstanced;
	switch (NewMode)
	{
	case ECustomWidgetInstancingType::EnableAndCreateDefaultInstance:
		// Emplaces default instance
		InstanceData.Reset();
		InstanceData.Emplace();
		bIsInstanced = true;
		break;
	case ECustomWidgetInstancingType::EnableInstancing:
		bIsInstanced = true;
		break;
	case ECustomWidgetInstancingType::NoInstancing:
	default:
		InstanceData.Reset();
		InstanceInfoPendingUpdate.Reset();
		bIsInstanced = false;
	}
	if (bOldInstancing != bIsInstanced)
	{
		ReleaseRenderResources();
		InvalidateWidget(IsValidForRendering(), true);
	}
}

bool SCustomMeshWidget::IsValidForRendering() const
{
	return IsValid(Mesh);
}

FIntRect SlateRectToIntRect(const FSlateRect SlateRect)
{
	return FIntRect(
		FMath::TruncToInt( FMath::Max(0.0f, SlateRect.Left) ),
		FMath::TruncToInt( FMath::Max(0.0f, SlateRect.Top) ),
		FMath::TruncToInt( FMath::Max(0.0f, SlateRect.Right) ), 
		FMath::TruncToInt( FMath::Max(0.0f, SlateRect.Bottom) ) 
	);
}

int32 SCustomMeshWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (IsValidForRendering())
	{
		if (MeshRenderer.IsValid())
		{
			const FSlateRect SlateCanvasRect = AllottedGeometry.GetRenderBoundingRect();
			const FSlateRect ClippedCanvasRect = SlateCanvasRect.IntersectionWith(MyCullingRect);
			const FIntRect CanvasRect = SlateRectToIntRect(SlateCanvasRect);
			const FIntRect ClippingRect = SlateRectToIntRect(ClippedCanvasRect);

			const FSlateRenderTransform& SlateTransform = AllottedGeometry.GetAccumulatedRenderTransform();

			// Draw mesh
			if (MeshRenderer->BeginRenderingCanvas(CanvasRect, ClippingRect, MeshTransform, SlateTransform))
			{
				FSlateDrawElement::MakeCustom( OutDrawElements, LayerId, MeshRenderer );
				++LayerId;
			}
		}
	}
	else
	{
		const FSlateBrush* BackgroundBrush = FAppStyle::GetBrush("Window.Background");
		FSlateDrawElement::MakeBox(
			OutDrawElements, 
			LayerId, 
			AllottedGeometry.ToPaintGeometry(FVector2f(AllottedGeometry.GetLocalSize().X, AllottedGeometry.GetLocalSize().Y), FSlateLayoutTransform()), 
			BackgroundBrush, 
			ESlateDrawEffect::DisabledEffect, 
			FLinearColor::White
		);
		++LayerId;

		// Draw some text telling the user how to get retime anchors.
		const FText NoRasterizerText = NSLOCTEXT("MeshWidgetPluginModule", "WidgetHaveNoMeshToDraw", "Mesh or material isn't set properly.");
		const FSlateFontInfo SmallLayoutFont = FCoreStyle::GetDefaultFontStyle("Regular", 8);
		const FLinearColor LabelColor = FLinearColor::White;

		// We have to measure the string so we can draw it centered on the window. 
		const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		const FVector2D TextLabelSize = FontMeasure->Measure(NoRasterizerText, SmallLayoutFont);

		const FPaintGeometry LabelGeometry = AllottedGeometry.ToPaintGeometry(
			FSlateLayoutTransform(
				FVector2D((AllottedGeometry.GetLocalSize().X - TextLabelSize.X) / 2.f, (AllottedGeometry.GetLocalSize().Y - TextLabelSize.Y) / 2)
			)
		);
		
		FSlateDrawElement::MakeText(
			OutDrawElements, 
			LayerId, 
			LabelGeometry, 
			NoRasterizerText,
			SmallLayoutFont,
			ESlateDrawEffect::None, 
			LabelColor
		);
		++LayerId;
	}
	return LayerId;
}

void SCustomMeshWidget::InvalidateWidget(bool bConsiderRedraw, bool bConsiderResize)
{
	EInvalidateWidgetReason Flags = EInvalidateWidgetReason::None;
	if (bConsiderResize && InvalidateBounds())
	{
		Flags |= EInvalidateWidgetReason::Layout;
	}
	if (bConsiderRedraw)
	{
		Flags |= EInvalidateWidgetReason::Paint;
	}
	if (Flags != EInvalidateWidgetReason::None)
	{
		Invalidate(Flags);
	}
}

bool SCustomMeshWidget::InvalidateBounds()
{
	FBox NewBounds(EForceInit::ForceInit);
	if (IsValid(Mesh))
	{
		FBox CachedMeshBounds = Mesh->GetBounds().GetBox();
		if (IsInstanced())
		{
			for (const FCustomMeshWidgetInstanceData& Instance : InstanceData)
			{
				NewBounds += CachedMeshBounds.TransformBy(Instance.GetInstanceTransform());
			}
			NewBounds = NewBounds.IsValid ? NewBounds.TransformBy(MeshTransform) : FBox(EForceInit::ForceInit);
		}
		else
		{
			NewBounds = CachedMeshBounds.TransformBy(MeshTransform);
		}
	}

	if (NewBounds != CachedBounds)
	{
		CachedBounds = NewBounds;
		return true;
	}
	return false;
}

FVector2D SCustomMeshWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(CachedBounds.GetSize());
}

void SCustomMeshWidget::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Mesh);
	Collector.AddReferencedObject(Material);
}

void FillInvalidationBits(TBitArray<>& InvalidatedInstances, TArrayView<const FInstanceUpdateBatch> InstanceInfoPendingUpdate)
{
	const int32 InstancesCount = InvalidatedInstances.Num();
	for (const FInstanceUpdateBatch& UpdateBatch : InstanceInfoPendingUpdate)
	{
		const int32 BaseIndex = FMath::Clamp(UpdateBatch.StartIndex, 0, InstancesCount - 1);
		const int32 RightIndex = FMath::Clamp(UpdateBatch.StartIndex + UpdateBatch.Length, 0, InstancesCount - 1);
		const int32 RunLength = FMath::Max(RightIndex, BaseIndex) - BaseIndex;
		InvalidatedInstances.SetRange(BaseIndex, RunLength, true);
	}
}

void BuildInvalidationRanges(TArray<FInstanceUpdateBatch>& InstanceInfoPendingUpdate, const TBitArray<>& InvalidatedInstances)
{
	int32 BaseIndex = 0;
	int32 NextInvalidationRegion = InvalidatedInstances.FindFrom(true, BaseIndex);
	InstanceInfoPendingUpdate.Reset();
	while (NextInvalidationRegion != INDEX_NONE)
	{
		FInstanceUpdateBatch Batch;
		Batch.Length = InvalidatedInstances.CountSetBits(NextInvalidationRegion);
		Batch.StartIndex = NextInvalidationRegion;
		InstanceInfoPendingUpdate.Add(Batch);
		BaseIndex = NextInvalidationRegion + Batch.Length;
		NextInvalidationRegion = InvalidatedInstances.FindFrom(true, BaseIndex);
	}
}

bool SCustomMeshWidget::UpdateInstance(int32 InstanceIndex, const FCustomMeshWidgetInstanceData& InInstanceData)
{
	if (!IsInstanced() || !InstanceData.IsValidIndex(InstanceIndex)) return false;
	TBitArray<> InvalidatedInstances;
	InvalidatedInstances.SetNum(InstanceData.Num(), false);
	FillInvalidationBits(InvalidatedInstances, MakeArrayView(InstanceInfoPendingUpdate));
	InstanceData[InstanceIndex] = InInstanceData;
	if (!InvalidatedInstances[InstanceIndex])
	{
		InvalidatedInstances[InstanceIndex] = true;
		BuildInvalidationRanges(InstanceInfoPendingUpdate, InvalidatedInstances);
	}
	InvalidateWidget(true, true);
	return true;
}

bool SCustomMeshWidget::UpdateInstanceTransform(int32 InstanceIndex, const FTransform& Transform)
{
	if (!IsInstanced() || !InstanceData.IsValidIndex(InstanceIndex)) return false;
	TBitArray<> InvalidatedInstances;
	InvalidatedInstances.SetNum(InstanceData.Num(), false);
	FillInvalidationBits(InvalidatedInstances, MakeArrayView(InstanceInfoPendingUpdate));
	InstanceData[InstanceIndex].SetTransform(Transform);
	if (!InvalidatedInstances[InstanceIndex])
	{
		InvalidatedInstances[InstanceIndex] = true;
		BuildInvalidationRanges(InstanceInfoPendingUpdate, InvalidatedInstances);
	}
	InvalidateWidget(true, true);
	return true;
}

void SCustomMeshWidget::ReleaseRenderResources()
{
	if (MeshRenderer.IsValid())
	{
		ENQUEUE_RENDER_COMMAND(SafeDeleteCustomMeshWidgetResources)([MeshRendererLatched = MeshRenderer](FRHICommandListImmediate& RHICmdList) mutable { MeshRendererLatched.Reset(); });
		MeshRenderer->ResetGameThreadInfo();
		MeshRenderer.Reset();
	}
}

FString SCustomMeshWidget::GetReferencerName() const
{
	return TEXT("SCustomMeshWidget");
}

void SCustomMeshWidget::CreateRenderResources()
{
	check(IsValidForRendering());
	if (IsInstanced())
	{
		MeshRenderer = MakeShared<FInstancedCustomMeshElement>(*this);
	}
	else
	{
		MeshRenderer = MakeShared<FCustomMeshSlateElement>(*this);
	}
}

void SCustomMeshWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SLeafWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Create required render resources
	if (!MeshRenderer.IsValid() && IsValidForRendering())
	{
		CreateRenderResources();
	}

	if (MeshRenderer.IsValid() && IsInstanceDataDirty())
	{
		if (MeshRenderer->SupportInstanceData())
		{
			for (const FInstanceUpdateBatch& UpdateBatch : InstanceInfoPendingUpdate)
			{
				MeshRenderer->SendInstanceUpdateRange(MakeArrayView<const FCustomMeshWidgetInstanceData>(&InstanceData[UpdateBatch.StartIndex], UpdateBatch.Length));
			}
		}
		InstanceInfoPendingUpdate.Reset();
	}

	if (MeshRenderer.IsValid() && IsAnimatedAttribute.Get())
	{
		MeshRenderer->SetAnimationTime(InCurrentTime, InDeltaTime);
	}
}