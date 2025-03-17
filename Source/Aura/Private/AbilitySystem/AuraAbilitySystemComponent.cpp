// Copyright XXX


#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"
#include "Aura/AuraLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "Interaction/PlayerInterface.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "Game/LoadScreenSaveGame.h"

void UAuraAbilitySystemComponent::AbilityActorInfoSet()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UAuraAbilitySystemComponent::ClientEffectApplied);
}

void UAuraAbilitySystemComponent::AddCharacherAbilitiesFromSaveData(ULoadScreenSaveGame* SaveData)
{
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	for (const FSavedAbility& Data : SaveData->SavedAbilities)
	{
		const TSubclassOf<UGameplayAbility> LoadedAbilityClass = Data.GameplayAbility;

		FGameplayAbilitySpec LoadedAbilitySpec(LoadedAbilityClass, Data.AbilityLevel);

		if (Data.AbilitySlot.IsValid())   LoadedAbilitySpec.DynamicAbilityTags.AddTag(Data.AbilitySlot);
		if (Data.AbilityStatus.IsValid()) LoadedAbilitySpec.DynamicAbilityTags.AddTag(Data.AbilityStatus);

		if (Data.AbilityType == GameplayTags.Abilities_Type_Offensive)
		{
			GiveAbility(LoadedAbilitySpec);
		}
		else if (Data.AbilityType == GameplayTags.Abilities_Type_Passive)
		{
			if (Data.AbilityStatus == GameplayTags.Abilities_Status_Equipped)
			{
				GiveAbilityAndActivateOnce(LoadedAbilitySpec);
			}
			else
			{
				GiveAbility(LoadedAbilitySpec);
			}
		}
		else
		{
			check(false);
		}
	}

	bStartupAbilitiesGiven = true;
	AbilitiesGivenDelegate.Broadcast();
}

void UAuraAbilitySystemComponent::AddCharacherAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
	for (TSubclassOf<UGameplayAbility> AbilityClass : StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec(AbilityClass, 1);
		if (UAuraGameplayAbility* AuraAbility = Cast<UAuraGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.DynamicAbilityTags.AddTag(AuraAbility->StartupInputTag);
			AbilitySpec.DynamicAbilityTags.AddTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
			GiveAbility(AbilitySpec);
		}
		//GiveAbilityAndActivateOnce(AbilitySpec);
	}

	bStartupAbilitiesGiven = true;
	AbilitiesGivenDelegate.Broadcast();
}

void UAuraAbilitySystemComponent::AddCharacherPassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupPassiveAbilities)
{
	for (TSubclassOf<UGameplayAbility> AbilityClass : StartupPassiveAbilities)
	{
		FGameplayAbilitySpec AbilitySpec(AbilityClass, 1);
		AbilitySpec.DynamicAbilityTags.AddTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
		GiveAbilityAndActivateOnce(AbilitySpec);
	}
}

// void UAuraAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
// {
// 	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
// 
// 	FDoRepLifetimeParams Params;
// 	Params.bIsPushBased = true;
// 	Params.Condition = COND_ReplayOrOwner;
// 	DOREPLIFETIME_WITH_PARAMS_FAST(UAuraAbilitySystemComponent, bStartupAbilitiesGiven, Params);
// }

void UAuraAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;

	const FGameplayAbilitySpec& AbilitySpec = ActivateAbilityByInputTag(InputTag);
	if (AbilitySpec.IsActive())
	{
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, AbilitySpec.Handle, AbilitySpec.ActivationInfo.GetActivationPredictionKey());
	}
}

void UAuraAbilitySystemComponent::AbilityInputTagHeld(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;

	ActivateAbilityByInputTag(InputTag);
}

FGameplayAbilitySpec UAuraAbilitySystemComponent::ActivateAbilityByInputTag(const FGameplayTag& InputTag)
{
	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag))
		{
			AbilitySpecInputPressed(AbilitySpec);
			if (!AbilitySpec.IsActive())
			{
				TryActivateAbility(AbilitySpec.Handle);
			}
			return AbilitySpec;
		}
	}
	return FGameplayAbilitySpec();
}

void UAuraAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;

	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag))
		{
			if (AbilitySpec.IsActive())
			{
				AbilitySpecInputReleased(AbilitySpec);
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, AbilitySpec.Handle, AbilitySpec.ActivationInfo.GetActivationPredictionKey());
			}
		}
	}
}

void UAuraAbilitySystemComponent::ForEachAbility(const FForEachAbility& Delegate)
{
	//FScopedAbilityListLock ActiveScopeLock(*this);
	ABILITYLIST_SCOPE_LOCK();

	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!Delegate.ExecuteIfBound(AbilitySpec))
		{
			UE_LOG(LogAura, Error, TEXT("Failed to execute delegate in %hs"), __FUNCTION__);
		}
	}
}

FGameplayTag UAuraAbilitySystemComponent::GetAbilityTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	if (AbilitySpec.Ability)
	{
		for (FGameplayTag Tag : AbilitySpec.Ability->AbilityTags)
		{
			if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName(TEXT("Abilities")))))
			{
				return Tag;
			}
		}
	}
	check(false);
	return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemComponent::GetInputTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	for (FGameplayTag Tag : AbilitySpec.DynamicAbilityTags)
	{
		if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName(TEXT("InputTag")))))
		{
			return Tag;
		}
	}
	return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemComponent::GetStatusFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	for (FGameplayTag Tag : AbilitySpec.DynamicAbilityTags)
	{
		if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName(TEXT("Abilities.Status")))))
		{
			return Tag;
		}
	}
	return FGameplayTag();
}

FGameplayAbilitySpec* UAuraAbilitySystemComponent::GetSpecFromAbilityTag(const FGameplayTag& AbilityTag)
{
	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		for (FGameplayTag Tag : AbilitySpec.Ability->AbilityTags)
		{
			if (Tag.MatchesTag(AbilityTag))
			{
				return &AbilitySpec;
			}
		}
	}
	return nullptr;
}

FGameplayAbilitySpec* UAuraAbilitySystemComponent::GetSpecWithSlot(const FGameplayTag& Slot)
{
	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilityHasSlot(AbilitySpec, Slot))
		{
			return &AbilitySpec;
		}
	}
	return nullptr;
}

FGameplayTag UAuraAbilitySystemComponent::GetStatusFromAbilityTag(const FGameplayTag& AbilityTag)
{
	if (const FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
	{
		return GetStatusFromSpec(*AbilitySpec);
	}
	return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemComponent::GetSlotInputTagFromAbilityTag(const FGameplayTag& AbilityTag)
{
	if (const FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
	{
		return GetInputTagFromSpec(*AbilitySpec);
	}
	return FGameplayTag();
}

bool UAuraAbilitySystemComponent::SlotIsEmpty(const FGameplayTag& Slot)
{
	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilityHasSlot(AbilitySpec, Slot))
		{
			return false;
		}
	}

	return true;
}

bool UAuraAbilitySystemComponent::IsPassiveAbility(const FGameplayAbilitySpec& AbilitySpec) const
{
	const UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(GetAvatarActor());
	checkf(AbilityInfo, TEXT("AAuraGameModeBase.AbilityInfo is only stored on Server"));
	const FGameplayTag AbilityTag = GetAbilityTagFromSpec(AbilitySpec);
	const FAuraAbilityInfo& Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
	const FGameplayTag AbilityType = Info.AbilityType;
	return AbilityType.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Type_Passive);
}

void UAuraAbilitySystemComponent::AssignSlotToAbility(FGameplayAbilitySpec& AbilitySpec, const FGameplayTag& Slot)
{
	// Clear this ability's slot
	ClearSlot(&AbilitySpec);
	// Now, assign this ability to this slot
	AbilitySpec.DynamicAbilityTags.AddTag(Slot);
}

void UAuraAbilitySystemComponent::MuticastActivatePassiveEffect_Implementation(const FGameplayTag& AbilityTag, bool bActivate)
{
	ActivatePassiveEffect.Broadcast(AbilityTag, bActivate);
}

void UAuraAbilitySystemComponent::UpgradeAttribute(const FGameplayTag& AttributeTag)
{
	if (GetAvatarActor()->Implements<UPlayerInterface>())
	{
		if (IPlayerInterface::Execute_GetAttributePoints(GetAvatarActor()) > 0)
		{
			ServerUpgradeAttribute(AttributeTag);
		}
	}
}

void UAuraAbilitySystemComponent::ServerUpgradeAttribute_Implementation(const FGameplayTag& AttributeTag)
{
	FGameplayEventData Payload;
	Payload.EventTag = AttributeTag;
	Payload.EventMagnitude = 1;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActor(), AttributeTag, Payload);

	if (GetAvatarActor()->Implements<UPlayerInterface>())
	{
		IPlayerInterface::Execute_AddToAttributePoints(GetAvatarActor(), -1);
	}
}

void UAuraAbilitySystemComponent::UpdateAbilityStatuses(int32 Level)
{
	UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(GetAvatarActor());
	if (AbilityInfo == nullptr) return;

	for (const FAuraAbilityInfo& Info : AbilityInfo->AbilityInformation)
	{
		if (!Info.AbilityTag.IsValid()) continue;
		if (!IsValid(Info.Ability)) continue;
		if (Level < Info.LevelRequirement) continue;
		// skip the ability that's already in our ability system component's activatable abilities.
		if (GetSpecFromAbilityTag(Info.AbilityTag) == nullptr)
		{
			const FGameplayTag StatusTag = FAuraGameplayTags::Get().Abilities_Status_Eligible;
			FGameplayAbilitySpec AbilitySpec(Info.Ability, 1);
			AbilitySpec.DynamicAbilityTags.AddTag(StatusTag);
			GiveAbility(AbilitySpec);
			MarkAbilitySpecDirty(AbilitySpec);
			ClientUpdateAbilityStatus(Info.AbilityTag, StatusTag, AbilitySpec.Level);
		}
	}
}

void UAuraAbilitySystemComponent::ServerSpendSpellPoint_Implementation(const FGameplayTag& AbilityTag)
{
	if (FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
	{
		//
		if (GetAvatarActor()->Implements<UPlayerInterface>())
		{
			IPlayerInterface::Execute_AddToSpellPoints(GetAvatarActor(), -1);
		}

		//
		const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
		FGameplayTag AbilityStatus = GetStatusFromSpec(*AbilitySpec);
		if (AbilityStatus.MatchesTagExact(GameplayTags.Abilities_Status_Eligible))
		{
			AbilitySpec->DynamicAbilityTags.RemoveTag(GameplayTags.Abilities_Status_Eligible);
			AbilitySpec->DynamicAbilityTags.AddTag(GameplayTags.Abilities_Status_Unlocked);
			AbilityStatus = GameplayTags.Abilities_Status_Unlocked;
		}
		else if (AbilityStatus.MatchesTagExact(GameplayTags.Abilities_Status_Equipped) || AbilityStatus.MatchesTagExact(GameplayTags.Abilities_Status_Unlocked))
		{
			AbilitySpec->Level += 1;
		}
		ClientUpdateAbilityStatus(AbilityTag, AbilityStatus, AbilitySpec->Level);
		// If you like to force it to replicate now rather than on next update
		MarkAbilitySpecDirty(*AbilitySpec);
	}
}

void UAuraAbilitySystemComponent::ServerEquipAbility_Implementation(const FGameplayTag& AbilityTag, const FGameplayTag& NewSlot)
{
	if (FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
	{
		const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
		const FGameplayTag PrevSlot = GetInputTagFromSpec(*AbilitySpec);
		const FGameplayTag Status = GetStatusFromSpec(*AbilitySpec);

		const bool IsDiffSlot = !PrevSlot.MatchesTagExact(NewSlot);
		const bool IsStatusValid = Status == GameplayTags.Abilities_Status_Equipped || Status == GameplayTags.Abilities_Status_Unlocked;
		if (IsDiffSlot && IsStatusValid)
		{
			// Handle activation/deactivation for passive abilities

			if (!SlotIsEmpty(NewSlot)) // There is an ability in this slot already. Deactivate and clear its slot.
			{
				FGameplayAbilitySpec* SpecWithSlot = GetSpecWithSlot(NewSlot);
				if (SpecWithSlot)
				{
					check(SpecWithSlot != AbilitySpec);
					check(!AbilityTag.MatchesTagExact(GetAbilityTagFromSpec(*SpecWithSlot)));
					// is that ability the same as this ability? If so, we can return early.
					if (AbilityTag.MatchesTagExact(GetAbilityTagFromSpec(*SpecWithSlot)))
					{
						check(false);
						ClientEquipAbility(AbilityTag, GetStatusFromSpec(*AbilitySpec), GetInputTagFromSpec(*AbilitySpec), PrevSlot);
						return;
					}

					if (IsPassiveAbility(*SpecWithSlot))
					{
						MuticastActivatePassiveEffect(GetAbilityTagFromSpec(*SpecWithSlot), false);
						DeactivatePassiveAbility.Broadcast(GetAbilityTagFromSpec(*SpecWithSlot));
					}

					ClearSlot(SpecWithSlot);
				}
				
				check(SlotIsEmpty(NewSlot));
				// Remove this InputTag (slot) from any Ability that has it.
				//ClearAbilitiesOfSlot(NewSlot);
			}

			if (!AbilityHasAnySlot(*AbilitySpec)) // Ability doesn't yet have a slot (it's not active)
			{
				if (IsPassiveAbility(*AbilitySpec))
				{
					check(!AbilitySpec->IsActive());
					if (!AbilitySpec->IsActive())
					{
						TryActivateAbility(AbilitySpec->Handle);
						MuticastActivatePassiveEffect(GetAbilityTagFromSpec(*AbilitySpec), true);
					}
				}

				AbilitySpec->DynamicAbilityTags.RemoveTag(GetStatusFromSpec(*AbilitySpec));
				AbilitySpec->DynamicAbilityTags.AddTag(GameplayTags.Abilities_Status_Equipped);
			}

			// Clear this ability's slot, just in case, it's a different slot
			// Now, assign this ability to this slot
			AssignSlotToAbility(*AbilitySpec, NewSlot);

			// Offensive/Passive Ability
			if (Status.MatchesTagExact(GameplayTags.Abilities_Status_Unlocked))
			{
				AbilitySpec->DynamicAbilityTags.RemoveTag(GameplayTags.Abilities_Status_Unlocked);
				AbilitySpec->DynamicAbilityTags.AddTag(GameplayTags.Abilities_Status_Equipped);
			}

			/*
			// Remove this InputTag (slot) from any Ability that has it.
			ClearAbilitiesOfSlot(NewSlot);
			// Clear this ability's slot, just in case, it's a different slot
			ClearSlot(AbilitySpec);
			// Now, assign this ability to this slot
			AbilitySpec->DynamicAbilityTags.AddTag(NewSlot);

			if (Status.MatchesTagExact(GameplayTags.Abilities_Status_Unlocked))
			{
				AbilitySpec->DynamicAbilityTags.RemoveTag(GameplayTags.Abilities_Status_Unlocked);
				AbilitySpec->DynamicAbilityTags.AddTag(GameplayTags.Abilities_Status_Equipped);
			}
			*/

			MarkAbilitySpecDirty(*AbilitySpec);

		}
			
		ClientEquipAbility(AbilityTag, GetStatusFromSpec(*AbilitySpec), GetInputTagFromSpec(*AbilitySpec), PrevSlot);
	}
}

void UAuraAbilitySystemComponent::ClientEquipAbility_Implementation(const FGameplayTag& AbilityTag, const FGameplayTag& Status, const FGameplayTag& Slot, const FGameplayTag& PreviousSlot)
{
	AbilityEquipped.Broadcast(AbilityTag, Status, Slot, PreviousSlot);
}

bool UAuraAbilitySystemComponent::GetDescriptionsByAbilityTag(const FGameplayTag& AbilityTag, FString& OutDescription, FString& OutNextLevelDescription)
{
	if (!AbilityTag.IsValid()) return false;
	if (AbilityTag.MatchesTagExact(FAuraGameplayTags::Get().Abilities_None)) return false;

	if (const FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
	{
		if (UAuraGameplayAbility* AuraAbility = Cast<UAuraGameplayAbility>(AbilitySpec->Ability))
		{
			OutDescription = AuraAbility->GetDescription(AbilitySpec->Level);
			OutNextLevelDescription = AuraAbility->GetNextLevelDescription(AbilitySpec->Level + 1);

			return true;
		}
	}

	const UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(GetAvatarActor());
	//checkf(AbilityInfo, TEXT("AAuraGameModeBase.AbilityInfo is only stored on Server"));
	if (AbilityInfo)
	{
		const FAuraAbilityInfo& Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag, true);
		OutDescription = UAuraGameplayAbility::GetLockedDescription(Info.LevelRequirement).Append(TEXT(" [Server]"));
	}
	OutNextLevelDescription = FString();

	return false;
}

void UAuraAbilitySystemComponent::ClearSlot(FGameplayAbilitySpec* AbilitySpec)
{
	const FGameplayTag Slot = GetInputTagFromSpec(*AbilitySpec);
	AbilitySpec->DynamicAbilityTags.RemoveTag(Slot);
	//MarkAbilitySpecDirty(*AbilitySpec);
}

void UAuraAbilitySystemComponent::ClearAbilitiesOfSlot(const FGameplayTag& Slot)
{
	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilityHasSlot(AbilitySpec, Slot))
		{
			ClearSlot(&AbilitySpec);
		}
	}
}

bool UAuraAbilitySystemComponent::AbilityHasSlot(const FGameplayAbilitySpec& AbilitySpec, const FGameplayTag& Slot)
{
	return AbilitySpec.DynamicAbilityTags.HasTagExact(Slot);
}

bool UAuraAbilitySystemComponent::AbilityHasAnySlot(const FGameplayAbilitySpec& AbilitySpec)
{
	return AbilitySpec.DynamicAbilityTags.HasTag(FGameplayTag::RequestGameplayTag(FName(TEXT("InputTag"))));
}

void UAuraAbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();

	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		const UGameplayAbility* SpecAbility = Spec.Ability;
		if (!SpecAbility)
		{
			return;
		}
	}

	if (!bStartupAbilitiesGiven)
	{
		bStartupAbilitiesGiven = true;
		AbilitiesGivenDelegate.Broadcast();
	}
}

void UAuraAbilitySystemComponent::ClientEffectApplied_Implementation(
	UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffecthandle)
{
	//GEngine->AddOnScreenDebugMessage(1, 8.f, FColor::Blue, FString("Effect Applied!"));

	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);

	EffectAssetTags.Broadcast(TagContainer);
}

void UAuraAbilitySystemComponent::ClientUpdateAbilityStatus_Implementation(const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag, int32 AbilityLevel)
{
	AbilityStatusChanged.Broadcast(AbilityTag, StatusTag, AbilityLevel);
}
