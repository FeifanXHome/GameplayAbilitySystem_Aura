// Copyright XXX


#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AuraGameplayTags.h"

void UAuraAbilitySystemComponent::AbilityActorInfoSet()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UAuraAbilitySystemComponent::EffectApplied);

	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	const FString& TagToString = GameplayTags.Attributes_Secondary_Armor.ToString();
	const FString& DebugMessage = FString::Printf(TEXT("Tag: %s"), *TagToString);
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Orange, DebugMessage);
}

void UAuraAbilitySystemComponent::EffectApplied(
	UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffecthandle)
{
	//GEngine->AddOnScreenDebugMessage(1, 8.f, FColor::Blue, FString("Effect Applied!"));

	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);

	EffectAssetTags.Broadcast(TagContainer);
}
