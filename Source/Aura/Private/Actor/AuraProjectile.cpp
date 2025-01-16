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
	PrimaryActorTick.bCanEverTick = false;

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
	// DamageEffectSpecHandle is only being set on server in UAuraProjectileSpell::SpawnProjectile
	// DamageEffectSpecHandle should be null on clients
	if (HasAuthority())
	{
		check(DamageEffectParams.SourceAbilitySystemComponent != nullptr);

		AActor* SourceAvatarActor = DamageEffectParams.SourceAbilitySystemComponent->GetAvatarActor();
		if (SourceAvatarActor == OtherActor) return;
		if (! UAuraAbilitySystemLibrary::IsNotFriend(SourceAvatarActor, OtherActor)) return;
	}
	else
	{
		check(DamageEffectParams.SourceAbilitySystemComponent == nullptr);

		// Case: Enemy
		AActor* owner = GetOwner();
		if (owner == OtherActor) return;
		if (! UAuraAbilitySystemLibrary::IsNotFriend(owner, OtherActor)) return;

		// Case: Aura
		const AAuraPlayerState* AuraPlayerState = GetOwner<AAuraPlayerState>();
		if (AuraPlayerState)
		{
			AActor* AvatarActor = (AuraPlayerState->GetAbilitySystemComponent()->GetAvatarActor());
			if (AvatarActor == OtherActor) return;
			if (! UAuraAbilitySystemLibrary::IsNotFriend(AvatarActor, OtherActor)) return;
		}
	}

	if (!bHit)
	{
		OnHit();
	}

	if (HasAuthority())
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
		{
			DamageEffectParams.TargetAbilitySystemComponent = TargetASC;
			DamageEffectParams.DeathImpulse   = GetActorForwardVector() * DamageEffectParams.DeathImpulseMagnitude;

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

		Destroy();
	}
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

