#include "ExtendedMergeUtilities.h"
#include "StaticMeshComponentLODInfo.h"
#include "ExtendedMeshMergingSettings.h"
#include "HierarchicalLODVolume.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Editor/EditorEngine.h"
#include "Selection.h"
#include "WrappedMeshBuilder.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "ISMPartition/ISMComponentBatcher.h"

#define LOCTEXT_NAMESPACE "FExtendedMeshInstancingTool"

void FMeshMergeUtilitiesEx::WrapComponentsToInstances(const TArray<USceneComponent*>& ComponentsToMerge, UWorld* World, ULevel* Level, const FWrappedMeshInstancingSettings& InSettings, bool bActuallyMerge /*= true*/, bool bReplaceSourceActors /* = false */, FText* OutResultsText /*= nullptr*/)
{
	auto HasInstanceVertexColors = [](UStaticMeshComponent* StaticMeshComponent)
	{
		for (const FStaticMeshComponentLODInfo& CurrentLODInfo : StaticMeshComponent->LODData)
		{
			if(CurrentLODInfo.OverrideVertexColors != nullptr || CurrentLODInfo.PaintedVertices.Num() > 0)
			{
				return true;
			}
		}

		return false;
	};

	UWrappedMeshBuilder* BuilderObjectInstance = InSettings.ActorBuilderInstance;
	FWrappedMeshBuilderContext Context;
	if (!BuilderObjectInstance)
	{
		if(OutResultsText != nullptr)
		{
			*OutResultsText = LOCTEXT("InstanceMergePredictedResultsNoBuilder", "No actors will be merged as builder class in not specified or unavailable.");
		}
		return;
	}
	else if (!BuilderObjectInstance->PrepareBuilderContext(Context, OutResultsText))
	{
		return;
	}

	// Gather valid components
	TArray<USceneComponent*> ValidComponents;
	for(USceneComponent* ComponentToMerge : ComponentsToMerge)
	{
		if (IsValid(ComponentToMerge) && BuilderObjectInstance->ShouldHarvest(ComponentToMerge))
		{
			if(UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(ComponentToMerge))
			{
				if( !InSettings.bSkipMeshesWithVertexColors || !HasInstanceVertexColors(StaticMeshComponent))
				{
					ValidComponents.Add(StaticMeshComponent);
				}
			}
			else if (InSettings.bGatherNonMeshComponents)
			{
				ValidComponents.Add(ComponentToMerge);
			}
		}
	}

	if(OutResultsText != nullptr)
	{
		*OutResultsText = LOCTEXT("InstanceMergePredictedResultsNone", "The current settings will not result in any instanced meshes being created");
	}

	if(ValidComponents.Num() > 0)
	{
		/** Helper struct representing a spawned ISMC */
		struct FComponentEntry
		{
			FComponentEntry(UStaticMeshComponent* InComponent)
			{
				StaticMesh = InComponent->GetStaticMesh();
				InComponent->GetUsedMaterials(Materials);
				bReverseCulling = InComponent->GetComponentTransform().ToMatrixWithScale().Determinant() < 0.0f;
				CollisionProfileName = InComponent->GetCollisionProfileName();
				CollisionEnabled = InComponent->GetCollisionEnabled();
				OriginalComponents.Add(InComponent);
			}

			bool operator==(const FComponentEntry& InOther) const
			{
				return 
					StaticMesh == InOther.StaticMesh &&
					Materials == InOther.Materials &&
					bReverseCulling == InOther.bReverseCulling && 
					CollisionProfileName == InOther.CollisionProfileName &&
					CollisionEnabled == InOther.CollisionEnabled;
			}

			UStaticMesh* StaticMesh;

			TArray<UMaterialInterface*> Materials;

			TArray<UStaticMeshComponent*> OriginalComponents;

			FName CollisionProfileName;

			bool bReverseCulling;

			ECollisionEnabled::Type CollisionEnabled;
		};

		/** Helper struct representing a spawned ISMC-containing actor */
		struct FActorEntry
		{
			FActorEntry(USceneComponent* InComponent, ULevel* InLevel)
				: MergedActor(nullptr), ActorBoundsWS(EForceInit::ForceInit), HLODVolume(nullptr)
			{
				// intersect with HLOD volumes if we have a level
				if(InLevel)
				{
					for (AActor* Actor : InLevel->Actors)
					{
						if (AHierarchicalLODVolume* HierarchicalLODVolume = Cast<AHierarchicalLODVolume>(Actor))
						{
							FBox BoundingBox = InComponent->Bounds.GetBox();
							FBox VolumeBox = HierarchicalLODVolume->GetComponentsBoundingBox(true);

							if (VolumeBox.IsInside(BoundingBox) || (HierarchicalLODVolume->bIncludeOverlappingActors && VolumeBox.Intersect(BoundingBox)))
							{
								HLODVolume = HierarchicalLODVolume;
								break;
							}
						}
					}
				}
			}

			FComponentEntry* AddComponent(USceneComponent* SceneComponent)
			{
				FBoxSphereBounds ComponentWorldBounds = SceneComponent->CalcBounds(SceneComponent->GetComponentToWorld());
				ActorBoundsWS += ComponentWorldBounds.GetBox();

				if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(SceneComponent))
				{
					FComponentEntry ComponentEntry(StaticMeshComponent);
					if(FComponentEntry* ExistingComponentEntry = ComponentEntries.FindByKey(ComponentEntry))
					{
						ExistingComponentEntry->OriginalComponents.Add(StaticMeshComponent);
						return ExistingComponentEntry;
					}
					else
					{
						return &ComponentEntries.Add_GetRef(ComponentEntry);
					}
				}
				else
				{
					NonMeshComponents.Add(SceneComponent);
				}
				return nullptr;
			}
			
			void MakeInstancingDecision(int32 MinInstancedComponents)
			{
				for (auto It = ComponentEntries.CreateIterator(); It; ++It)
				{
					int32 ItemsCount = 0;
					for (UStaticMeshComponent* OriginalComponent : It->OriginalComponents)
					{
						if (UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(OriginalComponent))
						{
							ItemsCount += ISM->GetInstanceCount();
						}
						else
						{
							ItemsCount++;
						}
					}
					if (ItemsCount < MinInstancedComponents)
					{
						NonMeshComponents.Append(It->OriginalComponents);
						It.RemoveCurrentSwap();
					}
				}
			}

			bool IsEmpty() const 
			{
				return ComponentEntries.IsEmpty() && NonMeshComponents.IsEmpty();
			}
			
			bool operator==(const FActorEntry& InOther) const
			{
				return HLODVolume == InOther.HLODVolume;
			}

			int32 CollectUniqueOriginalActors(TArray<AActor*>& Accum) const
			{
				int32 TotalComponentCount = 0;
				for(const FComponentEntry& ComponentEntry : ComponentEntries)
				{
					TotalComponentCount++;
					for(UStaticMeshComponent* OriginalComponent : ComponentEntry.OriginalComponents)
					{
						if(AActor* OriginalActor = OriginalComponent->GetOwner())
						{
							Accum.AddUnique(OriginalActor);
						}
					}
				}
				for(USceneComponent* NonMeshComponent : NonMeshComponents)
				{
					TotalComponentCount++;
					if(AActor* OriginalActor = NonMeshComponent->GetOwner())
					{
						Accum.AddUnique(OriginalActor);
					}
				}
				return TotalComponentCount;
			}

			AActor* MergedActor;
			FBox ActorBoundsWS;
			AHierarchicalLODVolume* HLODVolume;
			TArray<FComponentEntry> ComponentEntries;
			TArray<USceneComponent*> NonMeshComponents;
		};

		// Gather a list of components to merge
		TArray<FActorEntry> ActorEntries;
		for(USceneComponent* SceneComponent : ValidComponents)
		{
			int32 ActorEntryIndex = ActorEntries.AddUnique(FActorEntry(SceneComponent, InSettings.bUseHLODVolumes ? Level : nullptr));
			FActorEntry& ActorEntry = ActorEntries[ActorEntryIndex];
			ActorEntry.AddComponent(SceneComponent);
		}

		// Filter by component count
		for(FActorEntry& ActorEntry : ActorEntries)
		{
			ActorEntry.MakeInstancingDecision(InSettings.InstanceReplacementThreshold);
		}

		// Remove any empty actor entries
		ActorEntries.RemoveAll([](const FActorEntry& ActorEntry){ return ActorEntry.IsEmpty(); });

		int32 TotalComponentCount = 0;
		TArray<AActor*> ActorsToCleanUp;
		for(FActorEntry& ActorEntry : ActorEntries)
		{
			TotalComponentCount += ActorEntry.CollectUniqueOriginalActors(ActorsToCleanUp);
		}

		if(ActorEntries.Num() > 0)
		{
			if(OutResultsText != nullptr)
			{
				*OutResultsText = FText::Format(LOCTEXT("InstanceMergePredictedResults", "The current settings will result in {0} instanced static mesh components ({1} actors will be replaced)"), FText::AsNumber(TotalComponentCount), FText::AsNumber(ActorsToCleanUp.Num()));
			}
			
			if(bActuallyMerge)
			{
				// Create our actors
				const FScopedTransaction Transaction(LOCTEXT("PlaceInstancedActors", "Place Instanced Actor(s)"));
				Level->Modify(false);

 				FActorSpawnParameters Params;
 				Params.OverrideLevel = Level;

				// We now have the set of component data we want to apply
				for(FActorEntry& ActorEntry : ActorEntries)
				{
					Context.MergedBounds = ActorEntry.ActorBoundsWS;
					FTransform ActorRootTransformWS = FTransform(FMath::Lerp(ActorEntry.ActorBoundsWS.Min, ActorEntry.ActorBoundsWS.Max, InSettings.AnchorPoint));
					ActorEntry.MergedActor = Context.SpawnWrapperActor(World, ActorRootTransformWS, Params);
					BuilderObjectInstance->InitializeActor(ActorEntry.MergedActor, Context);

					USceneComponent* Root = ActorEntry.MergedActor->GetRootComponent();
					if (!IsValid(Root))
					{
						Root = NewObject<UInstancedStaticMeshComponent>(ActorEntry.MergedActor, InSettings.ISMComponentToUse.Get(), NAME_None, RF_Transactional);
						ActorEntry.MergedActor->SetRootComponent(Root);
						ActorEntry.MergedActor->RemoveOwnedComponent(Root);
						Root->CreationMethod = EComponentCreationMethod::Instance;
						ActorEntry.MergedActor->AddOwnedComponent(Root);
						Root->SetMobility(EComponentMobility::Static);
						Root->RegisterComponent();
					}
					
					// Spawn instanced meshes here
					for(const FComponentEntry& ComponentEntry : ActorEntry.ComponentEntries)
					{
						UInstancedStaticMeshComponent* NewComponent = nullptr;

						NewComponent = (UInstancedStaticMeshComponent*)ActorEntry.MergedActor->FindComponentByClass(InSettings.ISMComponentToUse.Get());

						if (NewComponent && NewComponent->PerInstanceSMData.Num() > 0)
						{
							NewComponent = nullptr;
						}

						if (NewComponent == nullptr)
						{
							NewComponent = NewObject<UInstancedStaticMeshComponent>(ActorEntry.MergedActor, InSettings.ISMComponentToUse.Get(), NAME_None, RF_Transactional);
							NewComponent->bHasPerInstanceHitProxies = true;
							NewComponent->OnComponentCreated();
							NewComponent->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);

							// Take 'instanced' ownership so it persists with this actor
							ActorEntry.MergedActor->RemoveOwnedComponent(NewComponent);
							NewComponent->CreationMethod = EComponentCreationMethod::Instance;
							ActorEntry.MergedActor->AddOwnedComponent(NewComponent);
						}

						NewComponent->SetStaticMesh(ComponentEntry.StaticMesh);
						for(int32 MaterialIndex = 0; MaterialIndex < ComponentEntry.Materials.Num(); ++MaterialIndex)
						{
							NewComponent->SetMaterial(MaterialIndex, ComponentEntry.Materials[MaterialIndex]);
						}
						NewComponent->SetReverseCulling(ComponentEntry.bReverseCulling);
						NewComponent->SetCollisionProfileName(ComponentEntry.CollisionProfileName);
						NewComponent->SetCollisionEnabled(ComponentEntry.CollisionEnabled);
						NewComponent->SetMobility(EComponentMobility::Static);

						FISMComponentBatcher ISMComponentBatcher;
						ISMComponentBatcher.Append(ComponentEntry.OriginalComponents);
						ISMComponentBatcher.InitComponent(NewComponent);						

						BuilderObjectInstance->InitializeComponent(NewComponent, ActorEntry.MergedActor, Context);
						if (!NewComponent->IsRegistered())
						{
							NewComponent->RegisterComponent();
						}
					}

					// Spawn the rest of components
					for(USceneComponent* OriginalComponent : ActorEntry.NonMeshComponents)
					{
						FTransform OriginalWorldTransform = OriginalComponent->GetComponentTransform();
						USceneComponent* NewComponent = DuplicateObject(OriginalComponent, ActorEntry.MergedActor);
						NewComponent->ClearFlags(RF_DefaultSubObject);
						NewComponent->OnComponentCreated();
						NewComponent->SetMobility(EComponentMobility::Static);
						NewComponent->AttachToComponent(Root, FAttachmentTransformRules::SnapToTargetIncludingScale);
						NewComponent->SetWorldTransform(OriginalWorldTransform);

						// Take 'instanced' ownership so it persists with this actor
						ActorEntry.MergedActor->RemoveOwnedComponent(NewComponent);
						NewComponent->CreationMethod = EComponentCreationMethod::Instance;
						ActorEntry.MergedActor->AddOwnedComponent(NewComponent);

						BuilderObjectInstance->InitializeComponent(NewComponent, ActorEntry.MergedActor, Context);
						if (!NewComponent->IsRegistered())
						{
							NewComponent->RegisterComponent();
						}
					}

					BuilderObjectInstance->FinalizeActor(ActorEntry.MergedActor, Context);
					World->UpdateCullDistanceVolumes(ActorEntry.MergedActor);
				}

				// Now clean up our original actors
				for(AActor* ActorToCleanUp : ActorsToCleanUp)
				{
					if (bReplaceSourceActors)
					{
						ActorToCleanUp->Destroy();
					}
					else
					{
						ActorToCleanUp->Modify();
						ActorToCleanUp->bIsEditorOnlyActor = true;
						ActorToCleanUp->SetHidden(true);
						ActorToCleanUp->bHiddenEd = true;
						ActorToCleanUp->SetIsTemporarilyHiddenInEditor(true);
					}
				}

				// pop a toast allowing selection
				auto SelectActorsLambda = [ActorEntries]()
				{ 
					GEditor->GetSelectedActors()->Modify();
					GEditor->GetSelectedActors()->BeginBatchSelectOperation();
					GEditor->SelectNone(false, true, false);

					for(const FActorEntry& ActorEntry : ActorEntries)
					{
						GEditor->SelectActor(ActorEntry.MergedActor, true, false, true);
					}

					GEditor->GetSelectedActors()->EndBatchSelectOperation();
				};

				// Always change selection if we removed the source actors,
				// Otherwise, allow selection change through notification
				if (bReplaceSourceActors)
				{
					SelectActorsLambda();
				}
				else
				{
					FNotificationInfo NotificationInfo(FText::Format(LOCTEXT("CreatedInstancedActorsMessage", "Created {0} Instanced Actor(s)"), FText::AsNumber(ActorEntries.Num())));
					NotificationInfo.Hyperlink = FSimpleDelegate::CreateLambda(SelectActorsLambda);
					NotificationInfo.HyperlinkText = LOCTEXT("SelectActorsHyperlink", "Select Actors");
					NotificationInfo.ExpireDuration = 5.0f;

					FSlateNotificationManager::Get().AddNotification(NotificationInfo);
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
