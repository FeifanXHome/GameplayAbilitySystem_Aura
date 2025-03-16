// Copyright XXX


#include "UI/WidgetController/OverlayWidgetController.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Player/AuraPlayerState.h"
#include "AbilitySystem/Data/LevelUpInfo.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/Data/AbilityInfo.h"

void UOverlayWidgetController::OnWidgetDestruct(UObject* AuraUserWidget)
{
	Super::OnWidgetDestruct(AuraUserWidget);

	OnHealthChanged.RemoveAll(AuraUserWidget);
	OnMaxHealthChanged.RemoveAll(AuraUserWidget);
	OnManaChanged.RemoveAll(AuraUserWidget);
	OnMaxManaChanged.RemoveAll(AuraUserWidget);
	MessageWidgetRowDelegate.RemoveAll(AuraUserWidget);
	OnXPPercentChangedDelegate.RemoveAll(AuraUserWidget);
	OnPlayerLevelChangedDelegate.RemoveAll(AuraUserWidget);
}

void UOverlayWidgetController::BroadcastInitialValues()
{
	OnHealthChanged.Broadcast(GetAuraAS()->GetHealth());
	OnMaxHealthChanged.Broadcast(GetAuraAS()->GetMaxHealth());
	OnManaChanged.Broadcast(GetAuraAS()->GetMana());
	OnMaxManaChanged.Broadcast(GetAuraAS()->GetMaxMana());

	OnXPChanged(GetAuraPS()->GetXP());
	OnPlayerLevelChangedDelegate.Broadcast(GetAuraPS()->GetPlayerLevel(), false);
}

void UOverlayWidgetController::BindCallbacksToDependencies()
{
	GetAuraPS()->OnXPChangedDelegate.AddUObject(this, &UOverlayWidgetController::OnXPChanged);
	GetAuraPS()->OnLevelChangedDelegate.
		AddLambda(
			[this](int32 NewLevel, bool bLevelUp)
			{
				OnPlayerLevelChangedDelegate.Broadcast(NewLevel, bLevelUp);
			}
	);

	AbilitySystemComponent
		->GetGameplayAttributeValueChangeDelegate(GetAuraAS()->GetHealthAttribute())
		.AddUObject(this, &UOverlayWidgetController::HealthChanged);
// 		.AddLambda(
// 			[this](const FOnAttributeChangeData& Data) 
// 			{
// 				OnHealthChanged.Broadcast(Data.NewValue);
// 			}
//		);
	AbilitySystemComponent
		->GetGameplayAttributeValueChangeDelegate(GetAuraAS()->GetMaxHealthAttribute())
		.AddLambda(
			[this](const FOnAttributeChangeData& Data) 
			{
				OnMaxHealthChanged.Broadcast(Data.NewValue);
			}
		);
	AbilitySystemComponent
		->GetGameplayAttributeValueChangeDelegate(GetAuraAS()->GetManaAttribute())
		.AddLambda(
			[this](const FOnAttributeChangeData& Data) 
			{
				OnManaChanged.Broadcast(Data.NewValue);
			}
		);
	AbilitySystemComponent
		->GetGameplayAttributeValueChangeDelegate(GetAuraAS()->GetMaxManaAttribute())
		.AddLambda(
			[this](const FOnAttributeChangeData& Data) 
			{
				OnMaxManaChanged.Broadcast(Data.NewValue);
			}
		);

	if (UAuraAbilitySystemComponent* AuraASC = GetAuraASC())
	{
		GetAuraASC()->AbilityEquipped.AddUObject(this, &UOverlayWidgetController::OnAbilityEquipped);

		if (AuraASC->bStartupAbilitiesGiven)
		{
			BroadcastAbilityInfo();
		}
		else
		{
			AuraASC->AbilitiesGivenDelegate.AddUObject(this, &UOverlayWidgetController::BroadcastAbilityInfo);
		}

		AuraASC->EffectAssetTags.AddLambda(
			[this](const FGameplayTagContainer& AssetTags)
			{
				for (const FGameplayTag& Tag : AssetTags)
				{
	 				// const FString Msg = FString::Printf(TEXT("GE Tag: %s"), *Tag.ToString());
	 				// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, Msg);

					// For example, say that Tag = Message.HealthPotion
					// "Message.HealthPotion".MatchesTag("Message") will return True, "Message".MatchesTag("Message.HealthPotion") will return False
					FGameplayTag MessageTag = FGameplayTag::RequestGameplayTag(FName("Message"));
					if (Tag.MatchesTag(MessageTag))
					{
						const FUIWidgetRow* Row = GetDataTableRowByTag<FUIWidgetRow>(MessageWidgetDataTable, Tag);
						MessageWidgetRowDelegate.Broadcast(*Row);
					}
				}
			}
		);
	}
}

void UOverlayWidgetController::HealthChanged(const FOnAttributeChangeData& Data) const
{
	OnHealthChanged.Broadcast(Data.NewValue);
}

void UOverlayWidgetController::OnXPChanged(int32 NewXP)
{
	const ULevelUpInfo* LevelUpInfo = GetAuraPS()->LevelUpInfo;
	checkf(LevelUpInfo, TEXT("Unabled to find LevelUpInfo. Please fill out AuraPlayerState Blueprint"));

	const int32 Level = LevelUpInfo->FindLevelForXP(NewXP);
	const int32 MaxLevel = LevelUpInfo->LevelUpInformation.Num();

	if (Level <= MaxLevel && Level > 0)
	{
		const int32 LevelUpRequirement = LevelUpInfo->LevelUpInformation[Level].LevelUpRequirement;
		const int32 PreviousLevelUpRequirement = LevelUpInfo->LevelUpInformation[Level-1].LevelUpRequirement;

		const int32 DeltaLevelRequirement = LevelUpRequirement - PreviousLevelUpRequirement;
		const int32 XPForThisLevel = NewXP - PreviousLevelUpRequirement;

		const float XPBarPercent = static_cast<float>(XPForThisLevel) / static_cast<float>(DeltaLevelRequirement);

		OnXPPercentChangedDelegate.Broadcast(XPBarPercent);
	}
}

void UOverlayWidgetController::OnAbilityEquipped(const FGameplayTag& AbilityTag, const FGameplayTag& Status, const FGameplayTag& Slot, const FGameplayTag& PreviousSlot)
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
}
