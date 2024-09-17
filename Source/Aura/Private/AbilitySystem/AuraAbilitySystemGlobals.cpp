// Copyright XXX


#include "AbilitySystem/AuraAbilitySystemGlobals.h"

UAuraAbilitySystemGlobals::UAuraAbilitySystemGlobals()
{
}

UAuraAbilitySystemGlobals::~UAuraAbilitySystemGlobals()
{
}

FAuraGameplayEffectContext* UAuraAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FAuraGameplayEffectContext();
}
