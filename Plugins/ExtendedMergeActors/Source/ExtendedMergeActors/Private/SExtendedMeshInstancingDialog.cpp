#include "SExtendedMeshInstancingDialog.h"
#include "ExtendedMeshInstancingTool.h"
#include "SlateOptMacros.h"

#include "IDetailsView.h"

//////////////////////////////////////////////////////////////////////////
// SWrappedMeshInstancingDialog
SWrappedMeshInstancingDialog::SWrappedMeshInstancingDialog()
{
	ComponentSelectionControl.ComponentFilter.BindStatic(&ComponentFilter);
}

SWrappedMeshInstancingDialog::~SWrappedMeshInstancingDialog()
{

}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SWrappedMeshInstancingDialog::Construct(const FArguments& InArgs, FWrappedMeshInstancingTool* InTool)
{
	checkf(InTool != nullptr, TEXT("Invalid owner tool supplied"));
	Tool = InTool;

	SMeshProxyCommonDialog::Construct(SMeshProxyCommonDialog::FArguments());

	InstancingSettings = UWrappedMeshInstancingSettingsObject::Get();
	SettingsView->SetObject(InstancingSettings);

	Reset();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FText SWrappedMeshInstancingDialog::GetPredictedResultsTextInternal() const
{
	return Tool->GetPredictedResultsText();
}
