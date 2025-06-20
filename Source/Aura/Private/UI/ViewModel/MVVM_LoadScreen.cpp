// Copyright XXX


#include "UI/ViewModel/MVVM_LoadScreen.h"
#include "UI/ViewModel/MVVM_LoadSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Game/AuraGameModeBase.h"
#include "Game/AuraGameInstance.h"

void UMVVM_LoadScreen::InitializeLoadSlots()
{
	LoadSlot_0 = NewObject<UMVVM_LoadSlot>(this, LoadSlotViewModelClass);
	LoadSlot_0->SlotIndex = 0;
	LoadSlot_0->LoadSlotName = FString("LoadSlot_0");
	LoadSlots.Add(0, LoadSlot_0);
	LoadSlot_1 = NewObject<UMVVM_LoadSlot>(this, LoadSlotViewModelClass);
	LoadSlot_1->SlotIndex = 1;
	LoadSlot_1->LoadSlotName = FString("LoadSlot_1");
	LoadSlots.Add(1, LoadSlot_1);
	LoadSlot_2 = NewObject<UMVVM_LoadSlot>(this, LoadSlotViewModelClass);
	LoadSlot_2->SlotIndex = 2;
	LoadSlot_2->LoadSlotName = FString("LoadSlot_2");
	LoadSlots.Add(2, LoadSlot_2);
}

UMVVM_LoadSlot* UMVVM_LoadScreen::GetLoadSlotViewModelByIndex(int32 Index) const
{
	return LoadSlots.FindChecked(Index);
}

void UMVVM_LoadScreen::NewSlotButtonPressed(int32 Slot, const FString& EnteredName)
{
	AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(this);
	AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(GameModeBase);
	//check(AuraGameMode);
	if (!IsValid(AuraGameMode))
	{
		GEngine->AddOnScreenDebugMessage(1, 15.f, FColor::Magenta, FString("Please switch to Single Player"));
		return;
	}

	UMVVM_LoadSlot* LoadSlot = LoadSlots[Slot];
	LoadSlot->SlotStatus = Taken;
	LoadSlot->SetMapName(AuraGameMode->DefaultMapName);
	LoadSlot->SetPlayerName(EnteredName);
	LoadSlot->SetPlayerLevel(1);
	LoadSlot->PlayerStartTag = AuraGameMode->DefaultPlayerStartTag;
	LoadSlot->MapAssetName = AuraGameMode->DefaultMap.ToSoftObjectPath().GetAssetName();

	AuraGameMode->SaveSlotData(LoadSlot, Slot);
	LoadSlot->InitializeSlot();

	UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(AuraGameMode->GetGameInstance());
	AuraGameInstance->LoadSlotName = LoadSlot->LoadSlotName;
	AuraGameInstance->LoadSlotIndex = LoadSlot->SlotIndex;
	AuraGameInstance->PlayerStartTag = AuraGameMode->DefaultPlayerStartTag;
}

void UMVVM_LoadScreen::NewGameButtonPressed(int32 Slot)
{
	LoadSlots[Slot]->SetWidgetSwitcherIndex.Broadcast(EnterName);
}

void UMVVM_LoadScreen::SelectSlotButtonPressed(int32 Slot)
{
	SlotSelected.Broadcast();

	for (const TTuple<int32, UMVVM_LoadSlot*> Pair : LoadSlots)
	{
		int32 SlotIndex = Pair.Key;
		UMVVM_LoadSlot* LoadSlot = Pair.Value;
		const bool bEnable = SlotIndex != Slot;
		LoadSlot->EnableSelectSlotButton.Broadcast(bEnable);
	}

	SelectedSlot = LoadSlots[Slot];
}

void UMVVM_LoadScreen::DeleteButtonPressed()
{
	if (IsValid(SelectedSlot))
	{
		AAuraGameModeBase::DeleteSlotData(SelectedSlot->LoadSlotName, SelectedSlot->SlotIndex);
		SelectedSlot->SlotStatus = Vacant;
		SelectedSlot->InitializeSlot();
		SelectedSlot->EnableSelectSlotButton.Broadcast(true);
	}
}

void UMVVM_LoadScreen::PlayButtonPressed()
{
	AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(this);
	AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(GameModeBase);
	//check(AuraGameMode);
	if (!IsValid(AuraGameMode))
	{
		GEngine->AddOnScreenDebugMessage(1, 15.f, FColor::Magenta, FString("Please switch to Single Player"));
		return;
	}

	if (IsValid(SelectedSlot))
	{
		UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(AuraGameMode->GetGameInstance());
		AuraGameInstance->PlayerStartTag = SelectedSlot->PlayerStartTag;
		AuraGameInstance->LoadSlotName = SelectedSlot->LoadSlotName;
		AuraGameInstance->LoadSlotIndex = SelectedSlot->SlotIndex;

		AuraGameMode->TravelToMap(SelectedSlot);
	}
}

void UMVVM_LoadScreen::LoadData()
{
	AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(this);
	AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(GameModeBase);
	//check(AuraGameMode);
	if (!IsValid(AuraGameMode))
	{
		GEngine->AddOnScreenDebugMessage(1, 15.f, FColor::Magenta, FString("Please switch to Single Player"));
		return;
	}

	for (const TTuple<int32, UMVVM_LoadSlot*> Pair : LoadSlots)
	{
		int32 SlotIndex = Pair.Key;
		UMVVM_LoadSlot* LoadSlot = Pair.Value;

		ULoadScreenSaveGame* SaveObject = AuraGameMode->GetSaveSlotData(LoadSlot->LoadSlotName, SlotIndex);

		LoadSlot->SlotStatus = SaveObject->SaveSlotStatus;
		LoadSlot->PlayerStartTag = SaveObject->PlayerStartTag;
		LoadSlot->SetPlayerName(SaveObject->PlayerName);
		LoadSlot->SetMapName(SaveObject->MapName);
		LoadSlot->SetPlayerLevel(SaveObject->PlayerLevel);
		LoadSlot->InitializeSlot();
	}
}
