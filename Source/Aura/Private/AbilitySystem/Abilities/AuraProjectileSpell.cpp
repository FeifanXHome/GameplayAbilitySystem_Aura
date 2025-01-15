// Copyright XXX


#include "AbilitySystem/Abilities/AuraProjectileSpell.h"
#include "Actor/AuraProjectile.h"
#include "Interaction/CombatInterface.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

void UAuraProjectileSpell::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	//HasAuthority(&ActivationInfo);
	//HasAuthority(&GetCurrentActivationInfo());

	//FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Event.Montage.FireBolt")), true);
	//UAbilityTask_WaitGameplayEvent* WaitGameplayEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, EventTag);
	//WaitGameplayEvent->EventReceived.AddDynamic(this, &UAuraProjectileSpell::OnReceivedEvent);
	//WaitGameplayEvent->ReadyForActivation();
}

void UAuraProjectileSpell::SpawnProjectile(const FVector& ProjectileTargetLocation, const FGameplayTag& SocketTag, bool bOverridePitch, float PitchOverride)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const bool bIsServer = AvatarActor->HasAuthority();
	if (!bIsServer) return;

	check(AvatarActor->Implements<UCombatInterface>());
	check(AvatarActor->GetClass()->ImplementsInterface(UCombatInterface::StaticClass()));
	const FVector SocketLocation = ICombatInterface::Execute_GetCombatSocketLocation(AvatarActor, SocketTag);

	FRotator Rotation = (ProjectileTargetLocation - SocketLocation).Rotation();
	Rotation.Pitch = bOverridePitch ? PitchOverride : 0.f;

	DoSpawnProjectile(ProjectileTargetLocation, SocketLocation, Rotation);
}

void UAuraProjectileSpell::DoSpawnProjectile(const FVector& ProjectileTargetLocation, const FVector& SocketLocation, FRotator& Rotation, AActor* HomingTaget)
{
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(SocketLocation);
	SpawnTransform.SetRotation(Rotation.Quaternion());

	AAuraProjectile* Projectile = GetWorld()->SpawnActorDeferred<AAuraProjectile>(
		ProjectileClass,
		SpawnTransform,
		GetOwningActorFromActorInfo(),
		Cast<APawn>(GetOwningActorFromActorInfo()),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	Projectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();

	// --- Homing ---
	if (HomingTaget && bLaunchHomingProjectiles)
	{
		if (HomingTaget->Implements<UCombatInterface>())
		{
			Projectile->ProjectileMovement->HomingTargetComponent = HomingTaget->GetRootComponent();
		}
		else
		{
			if (Projectile->HomingTagetSceneComponent.IsNull())
			{
				Projectile->HomingTagetSceneComponent = NewObject<USceneComponent>(USceneComponent::StaticClass());
			}
			Projectile->HomingTagetSceneComponent->SetWorldLocation(ProjectileTargetLocation);
			Projectile->ProjectileMovement->HomingTargetComponent = Projectile->HomingTagetSceneComponent;
		}
		Projectile->ProjectileMovement->HomingAccelerationMagnitude = FMath::FRandRange(HomingAccelerationMin, HomingAccelerationMax);
		Projectile->ProjectileMovement->bIsHomingProjectile = bLaunchHomingProjectiles;
	}
	// --- end Homing ---

	Projectile->FinishSpawning(SpawnTransform);
}
