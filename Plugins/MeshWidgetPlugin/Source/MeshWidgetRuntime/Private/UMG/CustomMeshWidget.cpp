#include "UMG/CustomMeshWidget.h"
#include "Slate/CustomMeshWidget.h"
#include "SlateMaterialBrush.h"

UCustomMeshWidget::UCustomMeshWidget(const FObjectInitializer& ObjectInitializer)
{
	Mesh = nullptr;
	Material = nullptr;
	MeshTransform = FTransform::Identity;
	bEnableInstancing = false;
}

void UCustomMeshWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	
	if (!MeshWidget.IsValid()) return;

	MeshWidget->SetStaticMesh(Mesh);
	MeshWidget->SetMaterial(Material);

	if (!MeshWidget->IsSameMeshTransform(MeshTransform))
	{
		MeshWidget->SetMeshTransform(MeshTransform);
	}

	if (bEnableInstancing != MeshWidget->IsInstanced())
	{
		MeshWidget->SetInstancingMode(bEnableInstancing ? ECustomWidgetInstancingType::EnableInstancing : ECustomWidgetInstancingType::NoInstancing);
		if (bEnableInstancing)
		{

		}
	}
}

void UCustomMeshWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	MeshWidget.Reset();
}

#if WITH_EDITOR
const FText UCustomMeshWidget::GetPaletteCategory()
{
	return NSLOCTEXT("MeshWidgetPluginModule", "Common", "Common");
}
#endif

TSharedRef<SWidget> UCustomMeshWidget::RebuildWidget()
{
	return SAssignNew(MeshWidget, SCustomMeshWidget).Mesh(Mesh);
}

void UCustomMeshWidget::SetStaticMesh(UStaticMesh* InMesh)
{
	if (MeshWidget.IsValid())
	{
		Mesh = InMesh;
		MeshWidget->SetStaticMesh(Mesh);
	}
}

UStaticMesh* UCustomMeshWidget::GetStaticMesh() const
{
	return MeshWidget.IsValid() ? MeshWidget->GetStaticMesh() : nullptr;
}

void UCustomMeshWidget::SetMaterial(UMaterialInterface* InMaterial)
{
	if (MeshWidget.IsValid())
	{
		MeshWidget->SetMaterial(InMaterial);
	}
}

UMaterialInterface* UCustomMeshWidget::GetMaterial()
{
	return (!Material && MeshWidget.IsValid()) ? MeshWidget->GetMaterial() : Material.Get(); 
}

void UCustomMeshWidget::SetAutoMaterial()
{
	if (MeshWidget.IsValid())
	{
		MeshWidget->SetMaterial(nullptr);
	}
}

bool UCustomMeshWidget::IsInstanced() const
{
	return MeshWidget.IsValid() ? MeshWidget->IsInstanced() : false;
}

bool UCustomMeshWidget::IsInstanceDataDirty() const
{
	return MeshWidget.IsValid() ? MeshWidget->IsInstanceDataDirty() : false;
}

int32 UCustomMeshWidget::GetNumInstances() const
{
	return MeshWidget.IsValid() ? MeshWidget->GetNumInstances() : 0;
}

bool UCustomMeshWidget::UpdateInstanceTransform(int32 InstanceIndex, const FTransform& Transform)
{
	if (MeshWidget.IsValid())
	{
		return MeshWidget->UpdateInstanceTransform(InstanceIndex, Transform);
	}
	return false;
}

FBox UCustomMeshWidget::GetLocalSpaceBounds() const
{
	return MeshWidget.IsValid() ? MeshWidget->GetLocalSpaceBounds() : FBox(EForceInit::ForceInit);
}

UMaterialInstanceDynamic* UCustomMeshWidget::GetDynamicMaterialInstance(UObject* Outer, bool bCreateSingleUser)
{
	return MeshWidget.IsValid() ? MeshWidget->GetDynamicMaterialInstance(Outer ? Outer : this, bCreateSingleUser) : nullptr;
}
