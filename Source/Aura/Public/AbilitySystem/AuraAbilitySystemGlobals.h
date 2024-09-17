// Copyright XXX

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "AuraAbilityTypes.h"
#include "AuraAbilitySystemGlobals.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()
	
	UAuraAbilitySystemGlobals();
	~UAuraAbilitySystemGlobals();

	/** Should allocate a project specific GameplayEffectContext struct. Caller is responsible for deallocation */
	virtual FAuraGameplayEffectContext* AllocGameplayEffectContext() const override;
};
