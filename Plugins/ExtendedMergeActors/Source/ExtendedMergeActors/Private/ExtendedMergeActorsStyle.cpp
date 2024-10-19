// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExtendedMergeActorsStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FExtendedMergeActorsStyle::StyleInstance = nullptr;

void FExtendedMergeActorsStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FExtendedMergeActorsStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FExtendedMergeActorsStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("ExtendedMergeActorsStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FExtendedMergeActorsStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("ExtendedMergeActorsStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("ExtendedMergeActors")->GetBaseDir() / TEXT("Resources"));

	Style->Set("ExtendedMergeActors.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));

	return Style;
}

void FExtendedMergeActorsStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FExtendedMergeActorsStyle::Get()
{
	return *StyleInstance;
}
