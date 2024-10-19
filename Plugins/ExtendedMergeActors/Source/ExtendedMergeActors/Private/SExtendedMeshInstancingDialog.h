#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"

#include "MergeProxyUtils/SMeshProxyCommonDialog.h"

class FWrappedMeshInstancingTool;
class UWrappedMeshInstancingSettingsObject;

/*-----------------------------------------------------------------------------
   SWrappedMeshInstancingDialog
-----------------------------------------------------------------------------*/
class SWrappedMeshInstancingDialog : public SMeshProxyCommonDialog
{
public:
	SLATE_BEGIN_ARGS(SWrappedMeshInstancingDialog)
	{
	}
	SLATE_END_ARGS()

public:
	/** **/
	SWrappedMeshInstancingDialog();
	~SWrappedMeshInstancingDialog();

	/** SWidget functions */
	void Construct(const FArguments& InArgs, FWrappedMeshInstancingTool* InTool);

protected:
	/** Refresh the predicted results text */;
	FText GetPredictedResultsTextInternal() const override;

private:
	/** Owning mesh instancing tool */
	FWrappedMeshInstancingTool* Tool;

	/** Cached pointer to mesh instancing setting singleton object */
	UWrappedMeshInstancingSettingsObject* InstancingSettings;
};
