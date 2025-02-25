// Copyright XXX


#include "Actor/AuraProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/AudioComponent.h"
#include "Aura/Aura.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interaction/CombatInterface.h"
#include "Player/AuraPlayerState.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"

AAuraProjectile::AAuraProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	/*
	I want to spawn a projectile on the server.
	I don't want to spawn this locally.
	I want this projectile to be a replicated actor.
	And that way if the server spawns it, then the server will be in charge of moving it, handling its location and all that good stuff,
	and the clients will just see a replicated version of the projectile.
	We're going to set bReplicates equal to true.
	So we now have a replicated projectile class.
	*/
	bReplicates = true;
	//SetReplicates(true); LogActor: Warning: SetReplicates called on non-initialized actor BP_FireBall_C_24. Directly setting bReplicates is the correct procedure for pre-init actors.
	SetReplicateMovement(true);

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Spheree"));
	Sphere->bHiddenInGame = false;
	Sphere->SetCollisionObjectType(ECC_Projectile);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(Sphere);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementt"));
	ProjectileMovement->InitialSpeed = 550.f;
	ProjectileMovement->MaxSpeed = 550.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	SetActorTickInterval(0.2);
}

void AAuraProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float dis = FVector::Dist(GetActorLocation(), LocationLastFrame);
	//FString msg2 = FString::Printf(TEXT("dis: %f"), dis);
	if (dis <= MinDistancePerFrame)
	{
		//UKismetSystemLibrary::PrintString(this, msg2, true, true, FLinearColor::Red, 3.f);
		OnHit();
		if (HasAuthority()) Destroy();
	}
	LocationLastFrame = GetActorLocation();
}

void AAuraProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(lifeSpan);
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &AAuraProjectile::OnSphereOverlap);
	
	LoopingSoundComponent = UGameplayStatics::SpawnSoundAttached(LoopingSound, GetRootComponent());

	// adjust the location on clients
	if (!HasAuthority())
	{
		const AAuraPlayerState* AuraPlayerState = GetOwner<AAuraPlayerState>();
		if (AuraPlayerState)
		{
			ICombatInterface* CombatInterface = Cast<ICombatInterface>(AuraPlayerState->GetAbilitySystemComponent()->GetAvatarActor());
			if (CombatInterface)
			{
				//const FVector SocketLocation = CombatInterface->Execute_GetCombatSocketLocation(AuraPlayerState->GetAbilitySystemComponent()->GetAvatarActor());
				//FString msg2 = FString::Printf(TEXT("BeginPlay Owner Location: %s, %p"), *SocketLocation.ToString(), CombatInterface);
				//UKismetSystemLibrary::PrintString(this, msg2, true, true, FLinearColor::Red, 3.f);
				//this->SetActorLocation(SocketLocation);
			}
		}
	}
}

void AAuraProjectile::Destroyed()
{
	if (!bHit && !HasAuthority())
	{
		OnHit();
	}
	
	if (IsValid(LoopingSoundComponent))
	{
		LoopingSoundComponent->Stop();
		LoopingSoundComponent->DestroyComponent();
	}
	// else{UKismetSystemLibrary::PrintString(this, FString(TEXT("Destroyed*******")), true, true, FLinearColor::Red, 3.f);}

	Super::Destroyed();
}

void AAuraProjectile::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (! IsValidOverlap(OtherActor))
	{
		return;
	}

	if (!bHit)
	{
		OnHit();
	}

	if (HasAuthority())
	{
		ApplyDamage(OtherActor);

		Destroy();
	}
}

void AAuraProjectile::ApplyDamageWithoutKnockback(AActor* TargetActor)
{
	FDamageEffectParams Params = DamageEffectParams;
	DamageEffectParams.KnockbackChance = 0;
	ApplyDamage(TargetActor);
	DamageEffectParams = Params;
}

/*void AAuraProjectile::ApplyDamage(AActor* OtherActor)
{
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
	{
		DamageEffectParams.TargetAbilitySystemComponent = TargetASC;
		DamageEffectParams.DeathImpulse = GetActorForwardVector() * DamageEffectParams.DeathImpulseMagnitude;

		const bool bKnockback = FMath::RandRange(1, 100) < DamageEffectParams.KnockbackChance;
		if (bKnockback)
		{
			FRotator Rotation = GetActorRotation();
			Rotation.Pitch = 45.f;

			const FVector KnockbackDirection = Rotation.Vector();
			const FVector KnockbackForce = KnockbackDirection * DamageEffectParams.KnockbackForceMagnitude;

			DamageEffectParams.KnockbackForce = KnockbackForce;
		}

		FGameplayEffectContextHandle EffectContextHandle = UAuraAbilitySystemLibrary::ApplyDamageEffect(DamageEffectParams);
		check(EffectContextHandle.IsValid());
	}
}*/

void AAuraProjectile::ApplyDamage(
	AActor* TargetActor,
	bool bOverrideKnockbackDirection, FVector KnockbackDirectionOverride, float KnockbackMagnitude,
	bool bOverrideDeathImpulse, FVector DeathImpulseDirectionOverride, float DeathImpulseMagnitude,
	bool bOverridePitch, float PitchOverride,
	bool bInIsRadial, FVector RadialOrigin, float RadialInnerRadius, float RadialOuterRadius
)
{
	if (TargetActor == nullptr) return;
	if (!TargetActor->Implements<UCombatInterface>()) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!IsValid(TargetASC))
	{
		check(IsValid(TargetASC));
		return;
	}

	DamageEffectParams.TargetAbilitySystemComponent = TargetASC;
	FDamageEffectParams Params = DamageEffectParams;

	check(IsValid(TargetActor));
	if (IsValid(TargetActor))
	{
		FRotator Rotation = GetActorRotation();
		Rotation.Pitch = bOverridePitch ? PitchOverride : 0;

		const FVector ToTarget = Rotation.Vector();
		Params.DeathImpulse = ToTarget * Params.DeathImpulseMagnitude;

		const bool bKnockback = FMath::RandRange(1, 100) < Params.KnockbackChance;
		if (bKnockback) Params.KnockbackForce = ToTarget * Params.KnockbackForceMagnitude;
	}

	if (bOverrideKnockbackDirection)
	{
		if (KnockbackMagnitude != 0.f) Params.KnockbackForceMagnitude = KnockbackMagnitude;

		KnockbackDirectionOverride.Normalize();
		Params.KnockbackForce = KnockbackDirectionOverride * Params.KnockbackForceMagnitude;
		if (bOverridePitch)
		{
			FRotator KnockbackRotation = KnockbackDirectionOverride.Rotation();
			KnockbackRotation.Pitch = PitchOverride;
			Params.KnockbackForce = KnockbackRotation.Vector() * Params.KnockbackForceMagnitude;
		}
	}

	if (bOverrideDeathImpulse)
	{
		if (DeathImpulseMagnitude != 0.f) Params.DeathImpulseMagnitude = DeathImpulseMagnitude;
		
		DeathImpulseDirectionOverride.Normalize();
		Params.DeathImpulse = DeathImpulseDirectionOverride * Params.DeathImpulseMagnitude;
		if (bOverridePitch)
		{
			FRotator DeathImpulseRotation = DeathImpulseDirectionOverride.Rotation();
			DeathImpulseRotation.Pitch = PitchOverride;
			Params.DeathImpulse = DeathImpulseRotation.Vector() * Params.DeathImpulseMagnitude;
		}
	}

	if (bInIsRadial)
	{
		Params.bIsRadialDamage = bInIsRadial;
		Params.RadialDamageOrigin = RadialOrigin;
		Params.RadialDamageInnerRadius = RadialInnerRadius;
		Params.RadialDamageOuterRadius = RadialOuterRadius;
	}

	//FString msg2 = FString::Printf(TEXT("ApplyDamage: %s"), *TargetActor->GetName());
	//UKismetSystemLibrary::PrintString(this, msg2, true, true, FLinearColor::Red, 3.f);

	FGameplayEffectContextHandle EffectContextHandle = UAuraAbilitySystemLibrary::ApplyDamageEffect(Params);
	check(EffectContextHandle.IsValid());
}

bool AAuraProjectile::IsValidOverlap(AActor* OtherActor)
{
	// DamageEffectSpecHandle is only being set on server in UAuraProjectileSpell::SpawnProjectile
	// DamageEffectSpecHandle should be null on clients
	if (HasAuthority())
	{
		check(DamageEffectParams.SourceAbilitySystemComponent != nullptr);

		AActor* SourceAvatarActor = DamageEffectParams.SourceAbilitySystemComponent->GetAvatarActor();
		if (SourceAvatarActor == OtherActor) return false;
		if (!UAuraAbilitySystemLibrary::IsNotFriend(SourceAvatarActor, OtherActor)) return false;
	}
	else
	{
		check(DamageEffectParams.SourceAbilitySystemComponent == nullptr);

		// Case: Enemy
		AActor* owner = GetOwner();
		if (owner == OtherActor) return false;
		if (!UAuraAbilitySystemLibrary::IsNotFriend(owner, OtherActor)) return false;

		// Case: Aura
		const AAuraPlayerState* AuraPlayerState = GetOwner<AAuraPlayerState>();
		if (AuraPlayerState)
		{
			AActor* AvatarActor = (AuraPlayerState->GetAbilitySystemComponent()->GetAvatarActor());
			if (AvatarActor == OtherActor) return false;
			if (!UAuraAbilitySystemLibrary::IsNotFriend(AvatarActor, OtherActor)) return false;
		}
	}

	return true;
}

void AAuraProjectile::OnHit()
{
	UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation(), FRotator::ZeroRotator);
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, GetActorLocation());

	if (IsValid(LoopingSoundComponent))
	{
		LoopingSoundComponent->Stop();
		LoopingSoundComponent->DestroyComponent();
	}
	// else{UKismetSystemLibrary::PrintString(this, FString(TEXT("OnSphereOverlap*******")), true, true, FLinearColor::Red, 3.f);}

	bHit = true;
}

