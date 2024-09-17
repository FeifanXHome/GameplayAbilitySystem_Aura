// Copyright XXX


#include "AuraAbilityTypes.h"

bool FAuraGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Super::NetSerialize(Ar, Map, bOutSuccess);
	Ar << bIsBlockedHit;
	Ar << bIsCriticalHit;
	return true;
}
