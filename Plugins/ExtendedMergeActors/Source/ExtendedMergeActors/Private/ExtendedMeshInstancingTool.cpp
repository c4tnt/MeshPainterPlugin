#include "ExtendedMeshInstancingTool.h"
#include "MergeProxyUtils/Utils.h"
#include "Misc/ScopedSlowTask.h"
#include "SExtendedMeshInstancingDialog.h"
#include "MeshMergeModule.h"
#include "ExtendedMergeUtilities.h"

#define LOCTEXT_NAMESPACE "FExtendedMeshInstancingTool"

bool UWrappedMeshInstancingSettingsObject::bInitialized = false;
UWrappedMeshInstancingSettingsObject* UWrappedMeshInstancingSettingsObject::DefaultSettings = nullptr;

EMergeComponentFilterResult ComponentFilter(USceneComponent* Component)
{
	if (UStaticMeshComponent* SMComponent = Cast<UStaticMeshComponent>(Component))
	{
		return SMComponent->GetStaticMesh() ? EMergeComponentFilterResult::Include : EMergeComponentFilterResult::List;
	}
	else if (Component->IsA<UPrimitiveComponent>())
	{
		return Component->IsEditorOnly() ? EMergeComponentFilterResult::List : EMergeComponentFilterResult::Include;
	}
	return EMergeComponentFilterResult::Exclude;
}

bool FExtendedMergeActorsTool::GetReplaceSourceActors() const
{
	return bReplaceSourceActors;
}

void FExtendedMergeActorsTool::SetReplaceSourceActors(bool bInReplaceSourceActors)
{
	bReplaceSourceActors = bInReplaceSourceActors;
}

bool FExtendedMergeActorsTool::RunMergeFromSelection()
{
	TArray<TSharedPtr<FMergeComponentData>> SelectionData;
	BuildMergeComponentDataFromSelection(SelectionData, FComponentSelectionControl::FComponentFilter::CreateStatic(&ComponentFilter));

	if (SelectionData.Num() == 0)
	{
		return false;
	}

	FString PackageName;
	if (GetPackageNameForMergeAction(GetDefaultPackageName(), PackageName))
	{
		return RunMerge(PackageName, SelectionData);
	}
	else
	{
		return false;
	}
}

bool FExtendedMergeActorsTool::RunMergeFromWidget()
{
	FString PackageName;
	if (GetPackageNameForMergeAction(GetDefaultPackageName(), PackageName))
	{
		return RunMerge(PackageName, GetSelectedComponentsInWidget());
	}
	else
	{
		return false;
	}
}

bool HasAtLeastOneStaticMesh(const TArray<TSharedPtr<FMergeComponentData>>& ComponentsData)
{
	for (const TSharedPtr<FMergeComponentData>& ComponentData : ComponentsData)
	{
		if (!ComponentData->bShouldIncorporate)
			continue;

		const bool bIsMesh = (Cast<UStaticMeshComponent>(ComponentData->Component.Get()) != nullptr);

		if (bIsMesh)
			return true;
	}

	return false;
}

bool FExtendedMergeActorsTool::CanMergeFromSelection() const
{
	TArray<TSharedPtr<FMergeComponentData>> SelectedComponents;
	BuildMergeComponentDataFromSelection(SelectedComponents, FComponentSelectionControl::FComponentFilter::CreateStatic(&ComponentFilter));
	return HasAtLeastOneStaticMesh(SelectedComponents);
}

bool FExtendedMergeActorsTool::CanMergeFromWidget() const
{
	const TArray<TSharedPtr<FMergeComponentData>>& SelectedComponents = GetSelectedComponentsInWidget();
	return HasAtLeastOneStaticMesh(SelectedComponents);
}

FWrappedMeshInstancingTool::FWrappedMeshInstancingTool()
{
	bAllowShapeComponents = false;
	SettingsObject = UWrappedMeshInstancingSettingsObject::Get();
}

FWrappedMeshInstancingTool::~FWrappedMeshInstancingTool()
{
	UWrappedMeshInstancingSettingsObject::Destroy();
	SettingsObject = nullptr;
}

TSharedRef<SWidget> FWrappedMeshInstancingTool::GetWidget()
{
	SAssignNew(InstancingDialog, SWrappedMeshInstancingDialog, this);
	return InstancingDialog.ToSharedRef();
}

FName FWrappedMeshInstancingTool::GetIconName() const
{
	return "MergeActors.MeshInstancingTool";
}

FText FWrappedMeshInstancingTool::GetToolNameText() const
{
	return LOCTEXT("FExtendedMeshInstancingToolName", "Wrap & Batch");
}

FText FWrappedMeshInstancingTool::GetTooltipText() const
{
	return LOCTEXT("MeshInstancingToolTooltip", "Will generate an actor with instanced static mesh component(s) wrapped by the single culling bound.");
}

FString FWrappedMeshInstancingTool::GetDefaultPackageName() const
{
	return FString();
}

const TArray<TSharedPtr<FMergeComponentData>>& FWrappedMeshInstancingTool::GetSelectedComponentsInWidget() const
{
	return InstancingDialog->GetSelectedComponents();
}

bool FWrappedMeshInstancingTool::RunMerge(const FString& PackageName, const TArray<TSharedPtr<FMergeComponentData>>& SelectedComponents)
{
	TArray<AActor*> Actors;
	TArray<ULevel*> UniqueLevels;

	BuildActorsListFromMergeComponentsData(SelectedComponents, Actors, &UniqueLevels);

	// This restriction is only for replacement of selected actors with merged mesh actor
	if (UniqueLevels.Num() > 1)
	{
		FText Message = NSLOCTEXT("UnrealEd", "FailedToInstanceActorsSublevels_Msg", "The selected actors should be in the same level");
		FText Title = NSLOCTEXT("UnrealEd", "FailedToInstanceActors_Title", "Unable to replace actors with instanced meshes");
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 2
		FMessageDialog::Open(EAppMsgType::Ok, Message, Title);
#else
		FMessageDialog::Open(EAppMsgType::Ok, Message, &Title);
#endif
		return false;
	}

	// Instance...
	{
		FScopedSlowTask SlowTask(0, LOCTEXT("MergingActorsSlowTask", "Instancing actors..."));
		SlowTask.MakeDialog();

		// Extracting static mesh components from the selected mesh components in the dialog
		TArray<USceneComponent*> ComponentsToMerge;

		for ( const TSharedPtr<FMergeComponentData>& SelectedComponent : SelectedComponents)
		{
			// Determine whether or not this component should be incorporated according the user settings
			if (SelectedComponent->bShouldIncorporate && SelectedComponent->Component.IsValid())
			{
				ComponentsToMerge.Add(SelectedComponent->Component.Get());
			}
		}

		if (ComponentsToMerge.Num())
		{
			// spawn the actor that will contain out instances
			UWorld* World = ComponentsToMerge[0]->GetWorld();
			checkf(World != nullptr, TEXT("Invalid World retrieved from Mesh components"));
			const bool bActuallyMerge = true;
			FMeshMergeUtilitiesEx::WrapComponentsToInstances(ComponentsToMerge, World, UniqueLevels[0], SettingsObject->Settings, bActuallyMerge, bReplaceSourceActors, nullptr);
		}
	}

	if (InstancingDialog)
	{
		InstancingDialog->Reset();
	}

	return true;
}

FText FWrappedMeshInstancingTool::GetPredictedResultsText()
{
	const TArray<TSharedPtr<FMergeComponentData>>& SelectedComponents = InstancingDialog->GetSelectedComponents();
	TArray<AActor*> Actors;
	TArray<ULevel*> UniqueLevels;

	BuildActorsListFromMergeComponentsData(SelectedComponents, Actors, &UniqueLevels);

	// This restriction is only for replacement of selected actors with merged mesh actor
	if (UniqueLevels.Num() > 1)
	{
		return NSLOCTEXT("UnrealEd", "FailedToInstanceActorsSublevels_Msg", "The selected actors should be in the same level");
	}

	// Extracting static mesh components from the selected mesh components in the dialog
	TArray<USceneComponent*> ComponentsToMerge;

	for ( const TSharedPtr<FMergeComponentData>& SelectedComponent : SelectedComponents)
	{
		// Determine whether or not this component should be incorporated according the user settings
		if (SelectedComponent->bShouldIncorporate)
		{
			ComponentsToMerge.Add(SelectedComponent->Component.Get());
		}
	}
		
	FText OutResultsText;
	if(ComponentsToMerge.Num() > 0 && ComponentsToMerge[0])
	{
		UWorld* World = ComponentsToMerge[0]->GetWorld();
		checkf(World != nullptr, TEXT("Invalid World retrieved from Mesh components"));
		const bool bActuallyMerge = false;
		FMeshMergeUtilitiesEx::WrapComponentsToInstances(ComponentsToMerge, World, UniqueLevels.Num() > 0 ? UniqueLevels[0] : nullptr, SettingsObject->Settings, bActuallyMerge, bReplaceSourceActors, &OutResultsText);
	}
	else
	{
		OutResultsText = LOCTEXT("InstanceMergePredictedResultsNone", "The current settings will not result in any instanced meshes being created");
	}
	return OutResultsText;
}
#undef LOCTEXT_NAMESPACE