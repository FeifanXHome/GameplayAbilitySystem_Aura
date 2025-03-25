// Copyright XXX


#include "Game/AuraGameModeBase.h"
#include "UI/ViewModel/MVVM_LoadSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Game/LoadScreenSaveGame.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Game/AuraGameInstance.h"
#include "Interaction/SaveInterface.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Aura/AuraLogChannels.h"

void AAuraGameModeBase::SaveSlotData(UMVVM_LoadSlot* LoadSlot, int32 SlotIndex)
{
	if (UGameplayStatics::DoesSaveGameExist(LoadSlot->LoadSlotName, SlotIndex))
	{
		UGameplayStatics::DeleteGameInSlot(LoadSlot->LoadSlotName, SlotIndex);
	}

	USaveGame* SaveGameObject = UGameplayStatics::CreateSaveGameObject(LoadScreenSaveGameClass);
	ULoadScreenSaveGame* LoadScreenSaveGame = Cast<ULoadScreenSaveGame>(SaveGameObject);
	LoadScreenSaveGame->PlayerName = LoadSlot->GetPlayerName();
	LoadScreenSaveGame->SaveSlotStatus = Taken;
	LoadScreenSaveGame->MapName = LoadSlot->GetMapName();
	LoadScreenSaveGame->PlayerStartTag = LoadSlot->PlayerStartTag;

	UGameplayStatics::SaveGameToSlot(LoadScreenSaveGame, LoadSlot->LoadSlotName, SlotIndex);
}

ULoadScreenSaveGame* AAuraGameModeBase::GetSaveSlotData(const FString& SlotName, int32 SlotIndex) const
{
	USaveGame* SaveGameObject = nullptr;

	if (UGameplayStatics::DoesSaveGameExist(SlotName, SlotIndex))
	{
		SaveGameObject = UGameplayStatics::LoadGameFromSlot(SlotName, SlotIndex);
	}
	else
	{
		SaveGameObject = UGameplayStatics::CreateSaveGameObject(LoadScreenSaveGameClass);
	}

	ULoadScreenSaveGame* LoadScreenSaveGame = Cast<ULoadScreenSaveGame>(SaveGameObject);
	return LoadScreenSaveGame;
}

void AAuraGameModeBase::DeleteSlotData(const FString& SlotName, int32 SlotIndex)
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, SlotIndex))
	{
		UGameplayStatics::DeleteGameInSlot(SlotName, SlotIndex);
	}
}

ULoadScreenSaveGame* AAuraGameModeBase::RetrieveInGameSaveData()
{
	UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(GetGameInstance());

	const FString InGameLoadSlotName = AuraGameInstance->LoadSlotName;
	const int32 InGameLoadSlotIndex = AuraGameInstance->LoadSlotIndex;

	return GetSaveSlotData(InGameLoadSlotName, InGameLoadSlotIndex);
}

void AAuraGameModeBase::SaveInGameProgressData(ULoadScreenSaveGame* SaveObject)
{
	if (SaveObject == nullptr) return;
	UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(GetGameInstance());

	const FString InGameLoadSlotName = AuraGameInstance->LoadSlotName;
	const int32 InGameLoadSlotIndex = AuraGameInstance->LoadSlotIndex;
	AuraGameInstance->PlayerStartTag = SaveObject->PlayerStartTag;

	UGameplayStatics::SaveGameToSlot(SaveObject, InGameLoadSlotName, InGameLoadSlotIndex);
}

void AAuraGameModeBase::SaveWorldState(UWorld* World, const FString& DestinationMapAssetName)
{
	ULoadScreenSaveGame* SaveGameObject = RetrieveInGameSaveData();
	if (SaveGameObject == nullptr)
	{
		UE_LOG(LogAura, Error, TEXT("Failed to load slot"), __FUNCTION__, *GetNameSafe(this));
		return;
	}

	if (!DestinationMapAssetName.IsEmpty())
	{
		SaveGameObject->MapAssetName = DestinationMapAssetName;
		SaveGameObject->MapName = GetMapNameFromMapAssetName(DestinationMapAssetName);
	}

	FString WorldName = World->GetMapName();
	WorldName.RemoveFromStart(World->StreamingLevelsPrefix);

	FSavedMap SavedMap = SaveGameObject->GetSavedMapWithMapName(WorldName); // no matter if existing.
	SavedMap.MapAssetName = WorldName;
	SavedMap.SavedActors.Empty(); // clear it out, we'll fill it in with "actors"

	for (FActorIterator It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)) continue;
		if (!Actor->Implements<USaveInterface>()) continue;
		
		FSavedActor SavedActor;
		SavedActor.ActorName = Actor->GetFName();
		SavedActor.Transform = Actor->GetTransform();

		// Serialize the variables with SaveGame specifier
		FMemoryWriter MemoryWriter(SavedActor.Bytes);
		FObjectAndNameAsStringProxyArchive Archive(MemoryWriter, true);
		Actor->Serialize(Archive);

		SavedMap.SavedActors.AddUnique(SavedActor);
	}

	SaveGameObject->AddOrUpdateSavedMap(SavedMap);

	SaveInGameProgressData(SaveGameObject);
}

void AAuraGameModeBase::LoadWorldState(UWorld* World)
{
	ULoadScreenSaveGame* SaveGameObject = RetrieveInGameSaveData();
	if (SaveGameObject == nullptr)
	{
		UE_LOG(LogAura, Error, TEXT("Failed to load slot"), __FUNCTION__, *GetNameSafe(this));
		return;
	}

	FString WorldName = World->GetMapName();
	WorldName.RemoveFromStart(World->StreamingLevelsPrefix);

	FSavedMap SavedMap;
	if (!SaveGameObject->GetSavedMapWithMapName(WorldName, SavedMap)) return;
	
	for (FActorIterator It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)) continue;
		if (!Actor->Implements<USaveInterface>()) continue;

		FSavedActor* SavedActor = SavedMap.SavedActors.FindByKey(Actor->GetFName());
		if (SavedActor == nullptr) continue;

		if (ISaveInterface::Execute_ShouldLoadTransform(Actor))
		{
			Actor->SetActorTransform(SavedActor->Transform);
		}

		// Deserialize the variables with SaveGame specifier
		FMemoryReader MemoryWriter(SavedActor->Bytes);
		FObjectAndNameAsStringProxyArchive Archive(MemoryWriter, true);
		Actor->Serialize(Archive); // converts binary bytes back into variables

		ISaveInterface::Execute_LoadActor(Actor);
	}
}

void AAuraGameModeBase::TravelToMap(UMVVM_LoadSlot* LoadSlot)
{
	const FString MapName = LoadSlot->GetMapName();
	const FString SlotName = LoadSlot->LoadSlotName;
	const int32 SlotIndex = LoadSlot->SlotIndex;
	
	TSoftObjectPtr<UWorld> Map = Maps[MapName];
	UGameplayStatics::OpenLevelBySoftObjectPtr(LoadSlot, Map);
}

FString AAuraGameModeBase::GetMapNameFromMapAssetName(const FString& MapAssetName) const
{
	for (auto& Map : Maps)
	{
		if (Map.Value.ToSoftObjectPath().GetAssetName() == MapAssetName)
		{
			return Map.Key;
		}
	}
	check(false);
	return FString();
}

AActor* AAuraGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(GetGameInstance());
	FName PlayerStartTag = AuraGameInstance->PlayerStartTag;

	AActor* SelectedActor = nullptr;
	UWorld* World = GetWorld();
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		APlayerStart* PlayerStart = *It;
		if (PlayerStart->PlayerStartTag == PlayerStartTag)
		{
			SelectedActor = PlayerStart;
			break;
		}
		else if (SelectedActor == nullptr)
		{
			SelectedActor = PlayerStart;
		}
	}

	return SelectedActor;
}

void AAuraGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	Maps.Add(DefaultMapName, DefaultMap);
}
