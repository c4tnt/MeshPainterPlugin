// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableViewBase.h"

enum class ECheckBoxState : uint8;

/** Data structure used to keep track of the selected mesh components, and whether or not they should be incorporated in the merge */
struct FMergeComponentData
{
	FMergeComponentData(USceneComponent* InComponent)
		: Component(InComponent)
		, bShouldIncorporate(true)
	{}

	/** Component extracted from selected actors */
	TWeakObjectPtr<USceneComponent> Component;
	/** Flag determining whether or not this component should be incorporated into the merge */
	bool bShouldIncorporate;
};

enum EMergeComponentFilterResult
{
	Exclude,	// Do not add to the list of components
	List,		// Add to the list but disable by default
	Include		// Add to the list and enable
};

struct FComponentSelectionControl
{
	DECLARE_DELEGATE_RetVal_OneParam(EMergeComponentFilterResult, FComponentFilter, USceneComponent*);

	/** Delegate for the creation of the list view item's widget */
	TSharedRef<ITableRow> MakeComponentListItemWidget(TSharedPtr<FMergeComponentData> ComponentData, const TSharedRef<STableViewBase>& OwnerTable);

	
	void UpdateSelectedCompnentsAndListBox();

	/** Stores the individual check box states for the currently selected mesh components */
	void StoreCheckBoxState();

	/** Updates SelectedMeshComponent array according to retrieved mesh components from editor selection*/
	void  UpdateSelectedStaticMeshComponents();

	/** List of mesh components extracted from editor selection */
	TArray<TSharedPtr<FMergeComponentData>> SelectedComponents;
	/** List view ui element */
	TSharedPtr<SListView<TSharedPtr<FMergeComponentData>>> ComponentsListView;
	/** Map of keeping track of checkbox states for each selected component (used to restore state when listview is refreshed) */
	TMap<USceneComponent*, ECheckBoxState> StoredCheckBoxStates;
	/** Component filted delegate */
	FComponentFilter ComponentFilter;

	/** Number of selected static mesh components */
	int32 NumSelectedMeshComponents = 0;
};

void BuildMergeComponentDataFromSelection(TArray<TSharedPtr<FMergeComponentData>>& OutComponentsData, FComponentSelectionControl::FComponentFilter ComponentFilter);
void BuildActorsListFromMergeComponentsData(const TArray<TSharedPtr<FMergeComponentData>>& InComponentsData, TArray<AActor*>& OutActors, TArray<ULevel*>* OutLevels = nullptr);
bool GetPackageNameForMergeAction(const FString& DefaultPackageName, FString& OutPackageName);
