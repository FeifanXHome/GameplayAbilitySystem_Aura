// Copyright XXX


#include "UI/ViewModel/MVVM_LoadScreen.h"
#include "UI/ViewModel/MVVM_LoadSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Game/AuraGameModeBase.h"

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
	LoadSlot->SetPlayerName(EnteredName);

	AuraGameMode->SaveSlotData(LoadSlot, Slot);
	LoadSlot->InitializeSlot();
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
		LoadSlot->SetPlayerName(SaveObject->PlayerName);
		LoadSlot->InitializeSlot();
	}
}
