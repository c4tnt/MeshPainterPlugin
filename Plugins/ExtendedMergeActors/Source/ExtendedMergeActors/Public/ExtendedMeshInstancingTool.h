#pragma once

#include "CoreMinimal.h"
#include "IMergeActorsTool.h"
#include "ExtendedMeshMergingSettings.h"
#include "MergeProxyUtils/Utils.h"

#include "ExtendedMeshInstancingTool.generated.h"

class SWrappedMeshInstancingDialog;

EMergeComponentFilterResult ComponentFilter(USceneComponent* Component);

/** Singleton wrapper to allow for using the setting structure in SSettingsView */
UCLASS(config = Engine)
class UWrappedMeshInstancingSettingsObject : public UObject
{
	GENERATED_BODY()
public:
	UWrappedMeshInstancingSettingsObject(const FObjectInitializer& ObjectInitializer)
	{
		Settings.InitializeWithinUObject(ObjectInitializer, this);
	}

	static UWrappedMeshInstancingSettingsObject* Get()
	{
		if (!bInitialized)
		{
			DefaultSettings = DuplicateObject(GetMutableDefault<UWrappedMeshInstancingSettingsObject>(), nullptr);
			DefaultSettings->AddToRoot();
			bInitialized = true;
		}

		return DefaultSettings;
	}

	static void Destroy()
	{
		if (bInitialized)
		{
			if (UObjectInitialized() && DefaultSettings)
			{
				DefaultSettings->RemoveFromRoot();
				DefaultSettings->MarkAsGarbage();
			}

			DefaultSettings = nullptr;
			bInitialized = false;
		}
	}

protected:
	static bool bInitialized;
	// This is a singleton, duplicate default object
	static UWrappedMeshInstancingSettingsObject* DefaultSettings;

public:
	UPROPERTY(EditAnywhere, meta = (ShowOnlyInnerProperties), Category = MergeSettings)
	FWrappedMeshInstancingSettings Settings;
};

/**
* Merge Actors tool base class
*/
class FExtendedMergeActorsTool : public IMergeActorsTool
{
public:
	/** IMergeActorsTool partial implementation */
	bool GetReplaceSourceActors() const override;

	void SetReplaceSourceActors(bool bReplaceSourceActors) override;

	bool RunMergeFromSelection() override;

	bool RunMergeFromWidget() override;

	bool CanMergeFromSelection() const override;

	bool CanMergeFromWidget() const override;

protected:
	/**
	* Perform the tool merge operation
	*/
	virtual bool RunMerge(const FString& PackageName, const TArray<TSharedPtr<FMergeComponentData>>& SelectedComponents) = 0;

	/**
	* Returns the selected components from the tool's widget
	*/
	virtual const TArray<TSharedPtr<FMergeComponentData>>& GetSelectedComponentsInWidget() const = 0;

protected:
	bool bReplaceSourceActors = false;
	bool bAllowShapeComponents = true;
};

/**
 * Mesh Instancing Tool
 */
class FWrappedMeshInstancingTool : public FExtendedMergeActorsTool
{
	friend class SMeshInstancingDialog;

public:

	FWrappedMeshInstancingTool();
	~FWrappedMeshInstancingTool();

	// IMergeActorsTool interface
	virtual TSharedRef<SWidget> GetWidget() override;
	virtual FName GetIconName() const override;
	virtual FText GetToolNameText() const override;
	virtual FText GetTooltipText() const override;
	virtual FString GetDefaultPackageName() const override;

	/** Runs the merging logic to determine predicted results */
	FText GetPredictedResultsText();

protected:
	virtual bool RunMerge(const FString& PackageName, const TArray<TSharedPtr<FMergeComponentData>>& SelectedComponents) override;
	virtual const TArray<TSharedPtr<FMergeComponentData>>& GetSelectedComponentsInWidget() const override;

private:
	/** Pointer to the mesh instancing dialog containing settings for the merge */
	TSharedPtr<SWrappedMeshInstancingDialog> InstancingDialog;

	/** Pointer to singleton settings object */
	UWrappedMeshInstancingSettingsObject* SettingsObject;
};
