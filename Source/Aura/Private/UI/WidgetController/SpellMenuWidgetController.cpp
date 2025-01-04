// Copyright XXX


#include "UI/WidgetController/SpellMenuWidgetController.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "Player/AuraPlayerState.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"

void USpellMenuWidgetController::BroadcastInitialValues()
{
	BroadcastAbilityInfo();
	OnSpellPointsChangedDelegate.Broadcast(GetAuraPS()->GetSpellPoints());
}

void USpellMenuWidgetController::BindCallbacksToDependencies()
{
	GetAuraPS()->OnSpellPointsChangedDelegate.AddLambda(
		[this](int32 SpellPoints)
		{
			OnSpellPointsChangedDelegate.Broadcast(SpellPoints);

			CurrentSpellPoints = SpellPoints;
			BroadcastSpellGlobeSelectedDelegate(SelectedAbility.Ability, SelectedAbility.Status);
		}
	);

	GetAuraASC()->AbilityEquipped.AddUObject(this, &USpellMenuWidgetController::OnAbilityEquipped);

	GetAuraASC()->AbilityStatusChanged.AddLambda(
		[this](const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag, int32 NewAbilityLevel)
		{
			//
			if (AbilityInfo)
			{
				FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
				Info.StatusTag = StatusTag;

				AbilityInfoDelegate.Broadcast(Info);
			}

			//
			if (SelectedAbility.Ability.MatchesTagExact(AbilityTag))
			{
				SelectedAbility.Status = StatusTag;
				BroadcastSpellGlobeSelectedDelegate(SelectedAbility.Ability, SelectedAbility.Status);
			}
		}
	);
}

void USpellMenuWidgetController::SpellGlobeSelected(const FGameplayTag& AbilityTag)
{
	BroadcastStopWaitingForEquipDelegate(SelectedAbility.Ability);

	const FAuraGameplayTags GameplayTags = FAuraGameplayTags::Get();
	const int32 SpellPoints = GetAuraPS()->GetSpellPoints();
	const FGameplayAbilitySpec* AbilitySpec = GetAuraASC()->GetSpecFromAbilityTag(AbilityTag);
	FGameplayTag AbilityStatus;

	const bool bTagValid = AbilityTag.IsValid();
	const bool bTagNone = AbilityTag.MatchesTag(GameplayTags.Abilities_None);
	const bool bSpecValid = AbilitySpec != nullptr;

	if (!bTagValid || bTagNone || !bSpecValid)
	{
		AbilityStatus = GameplayTags.Abilities_Status_Locked;
	}
	else
	{
		AbilityStatus = GetAuraASC()->GetStatusFromSpec(*AbilitySpec);
	}

	SelectedAbility.Ability = AbilityTag;
	SelectedAbility.Status = AbilityStatus;
	CurrentSpellPoints = SpellPoints;

	BroadcastSpellGlobeSelectedDelegate(SelectedAbility.Ability, SelectedAbility.Status);
}

void USpellMenuWidgetController::SpendPointButtonPressed()
{
	GetAuraASC()->ServerSpendSpellPoint(SelectedAbility.Ability);
}

void USpellMenuWidgetController::GlobeDeselect()
{
	BroadcastStopWaitingForEquipDelegate(SelectedAbility.Ability);

	SelectedAbility = { FAuraGameplayTags::Get().Abilities_None, FAuraGameplayTags::Get().Abilities_Status_Locked };
	BroadcastSpellGlobeSelectedDelegate(SelectedAbility.Ability, SelectedAbility.Status);
}

void USpellMenuWidgetController::EquipButtonPressed()
{
	const FGameplayTag AbilityType = AbilityInfo->FindAbilityInfoForTag(SelectedAbility.Ability).AbilityType;
	
	WaitForEquipDelegate.Broadcast(AbilityType);
	bWaitingForEquipSelection = true;

	// Keep track of the currently selected ability's input tag if it's equipped
	const FGameplayTag SelectedStatus = GetAuraASC()->GetStatusFromAbilityTag(SelectedAbility.Ability);
	if (SelectedStatus.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Status_Equipped))
	{
		SelectedSlot = GetAuraASC()->GetInputTagFromAbilityTag(SelectedAbility.Ability);
	}
}

void USpellMenuWidgetController::SpellRowGlobePressed(const FGameplayTag& SlotTag, const FGameplayTag& AbilityType)
{
	if (!bWaitingForEquipSelection) return;
	// Check selected ability against the slot's ability type.
	// (don't equip an offensive spell to a passive slot, vice versa)
	const FGameplayTag SelectedAbilityType = AbilityInfo->FindAbilityInfoForTag(SelectedAbility.Ability).AbilityType;
	if (!SelectedAbilityType.MatchesTagExact(AbilityType)) return;
	
	GetAuraASC()->ServerEquipAbility(SelectedAbility.Ability, SlotTag);
}

void USpellMenuWidgetController::OnAbilityEquipped(const FGameplayTag& AbilityTag, const FGameplayTag& Status, const FGameplayTag& Slot, const FGameplayTag& PreviousSlot)
{
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	
	FAuraAbilityInfo LastSlotInfo;
	LastSlotInfo.StatusTag = GameplayTags.Abilities_Status_Unlocked;
	LastSlotInfo.InputTag = PreviousSlot;
	LastSlotInfo.AbilityTag = GameplayTags.Abilities_None;
	// Broadcast empty info if PreviousSlot is a valid slot. Only if equipping an already-equipped spell
	AbilityInfoDelegate.Broadcast(LastSlotInfo);

	FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
	Info.StatusTag = Status;
	Info.InputTag = Slot;
	AbilityInfoDelegate.Broadcast(Info);

	BroadcastStopWaitingForEquipDelegate(AbilityTag);
	SpellGlobeReassignDelegate.Broadcast(AbilityTag);
	//GlobeDeselect(); called from SpellGlobeReassignDelegate_Event (WBP_SpellGlobe_Button)
}

void USpellMenuWidgetController::BroadcastStopWaitingForEquipDelegate(const FGameplayTag& Ability)
{
	if (!bWaitingForEquipSelection) return;

	const FGameplayTag AbilityType = AbilityInfo->FindAbilityInfoForTag(Ability).AbilityType;

	StopWaitingForEquipDelegate.Broadcast(AbilityType);
	bWaitingForEquipSelection = false;
}

void USpellMenuWidgetController::BroadcastSpellGlobeSelectedDelegate(const FGameplayTag& Ability, const FGameplayTag& Status)
{
	bool bEnableSpendPointsButton = false;
	bool bEnableEquipButton = false;
	ShouldEnableButtons(Status, CurrentSpellPoints, bEnableSpendPointsButton, bEnableEquipButton);

	FString Description;
	FString NextLevelDescription;

	FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(Ability);
	if (Info.AbilityTag.IsValid())
	{
		Description = UAuraGameplayAbility::GetLockedDescription(Info.LevelRequirement).Append(TEXT(" [Client]"));
	}
	GetAuraASC()->GetDescriptionsByAbilityTag(Ability, Description, NextLevelDescription);

	OnSpellGlobeSelectedDelegate.Broadcast(bEnableSpendPointsButton, bEnableEquipButton, Description, NextLevelDescription);
}

void USpellMenuWidgetController::ShouldEnableButtons(const FGameplayTag& AbilityStatus, int32 SpellPoints, 
	bool& bShouldEnableSpendPointsButton, bool& bShouldEnableEquipButton)
{
	const FAuraGameplayTags GameplayTags = FAuraGameplayTags::Get();

	bShouldEnableSpendPointsButton = false;
	bShouldEnableEquipButton = false;

	if (AbilityStatus.MatchesTagExact(GameplayTags.Abilities_Status_Equipped))
	{
		bShouldEnableEquipButton = true;
		if (SpellPoints > 0) bShouldEnableSpendPointsButton = true;
	}
	else if (AbilityStatus.MatchesTagExact(GameplayTags.Abilities_Status_Eligible))
	{
		if (SpellPoints > 0) bShouldEnableSpendPointsButton = true;
	}
	else if (AbilityStatus.MatchesTagExact(GameplayTags.Abilities_Status_Unlocked))
	{
		bShouldEnableEquipButton = true;
		if (SpellPoints > 0) bShouldEnableSpendPointsButton = true;
	}
	else if (AbilityStatus.MatchesTagExact(GameplayTags.Abilities_Status_Locked))
	{

	}

}
