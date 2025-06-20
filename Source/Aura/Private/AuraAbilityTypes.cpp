// Copyright XXX


#include "AuraAbilityTypes.h"

bool FAuraGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Super::NetSerialize(Ar, Map, bOutSuccess);

	Ar << bIsBlockedHit;
	Ar << bIsCriticalHit;

	Ar << bIsSuccessfulDebuff;
	if (bIsSuccessfulDebuff)
	{
		Ar << DebuffDamage;
		Ar << DebuffFrequency;
		Ar << DebuffDuration;
	}

	Ar << DamageType;
	Ar << DeathImpulse;
	Ar << KnockbackForce;

	Ar << bIsRadialDamage;
	if (bIsRadialDamage)
	{
		Ar << RadialDamageInnerRadius;
		Ar << RadialDamageOuterRadius;
		Ar << RadialDamageOrigin;
	}

	return true;
}
