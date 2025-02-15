// Copyright XXX


#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Interaction/CombatInterface.h"

void UAuraDamageGameplayAbility::CauseDamage(AActor* TargetActor)
{
	if (TargetActor == nullptr) return;
	if (UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor) == nullptr) return;
	if (TargetActor->Implements<UCombatInterface>() && ICombatInterface::Execute_IsDead(TargetActor))
	{
		return;
	}

	FGameplayEffectSpecHandle DamageSpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	//for (TTuple<FGameplayTag, FScalableFloat>& Pair : DamagTypes)
	//{
	//	const float ScaledDamage = Pair.Value.GetValueAtLevel(GetAbilityLevel());
	//	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageSpecHandle, Pair.Key, ScaledDamage);
	//}
	const float ScaledDamage = GetDamageAtLevel();
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageSpecHandle, DamageType, ScaledDamage);

	//UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetAvatarActorFromActorInfo());
	//UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	// Option 1: TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data);
	// Option 2: SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data, TargetASC);
	// Option 3: 
	GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(
		*DamageSpecHandle.Data, 
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor)
	);
}

void UAuraDamageGameplayAbility::CauseDamageWithContext(AActor* TargetActor)
{
	if (TargetActor == nullptr) return;
	if (UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor) == nullptr) return;
	if (TargetActor->Implements<UCombatInterface>() && ICombatInterface::Execute_IsDead(TargetActor))
	{
		return;
	}

	FDamageEffectParams DamageEffectParams = MakeDamageEffectParamsFromClassDefaults(TargetActor);
	FGameplayEffectContextHandle EffectContextHandle = UAuraAbilitySystemLibrary::ApplyDamageEffect(DamageEffectParams);
	check(EffectContextHandle.IsValid());
}

FDamageEffectParams UAuraDamageGameplayAbility::MakeDamageEffectParamsFromClassDefaults(AActor* TargetActor) const
{
	FDamageEffectParams Params;

	Params.WorldContextObject = GetAvatarActorFromActorInfo();
	Params.DamageGameplayEffectClass = DamageEffectClass;
	Params.SourceAbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	Params.TargetAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	Params.BaseDamage		= GetDamageAtLevel();
	Params.AbilityLevel		= GetAbilityLevel();
	Params.DamageType		= DamageType;
	Params.DebuffChance		= DebuffChance;
	Params.DebuffDamage		= DebuffDamage;
	Params.DebuffDuration	= DebuffDuration;
	Params.DebuffFrequency	= DebuffFrequency;
	Params.DeathImpulseMagnitude   = DeathImpulseMagnitude;
	Params.KnockbackForceMagnitude = KnockbackForceMagnitude;
	Params.KnockbackChance		   = KnockbackChance;

	if (IsValid(TargetActor))
	{
		FRotator Rotation = (TargetActor->GetActorLocation() - GetAvatarActorFromActorInfo()->GetActorLocation()).Rotation();
		Rotation.Pitch = 45.f;

		const FVector ToTarget = Rotation.Vector();
		Params.DeathImpulse    = ToTarget * DeathImpulseMagnitude;

		const bool bKnockback = FMath::RandRange(1, 100) < Params.KnockbackChance;
		if (bKnockback) Params.KnockbackForce = ToTarget * KnockbackForceMagnitude;
	}

	if (bIsRadialDamage)
	{
		Params.bIsRadialDamage = bIsRadialDamage;
		Params.RadialDamageInnerRadius = RadialDamageInnerRadius;
		Params.RadialDamageOuterRadius = RadialDamageOuterRadius;
		Params.RadialDamageOrigin = RadialDamageOrigin;
	}

	return Params;
}

float UAuraDamageGameplayAbility::GetDamageAtLevel() const
{
	return Damage.GetValueAtLevel(GetAbilityLevel());
}

//float UAuraDamageGameplayAbility::GetDamageByDamageType(float InLevel, const FGameplayTag& DamageType)
//{
//	checkf(DamagTypes.Contains(DamageType), TEXT("GameplayAbility [%s] does not contain DamageType [%s]"), *GetNameSafe(this), *DamageType.ToString());
//	return DamagTypes[DamageType].GetValueAtLevel(InLevel);
//}
