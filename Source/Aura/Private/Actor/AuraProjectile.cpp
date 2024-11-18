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
	if (!DamageEffectSpecHandle.IsValid())
	{
		check(!HasAuthority());

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
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation(), FRotator::ZeroRotator);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, GetActorLocation());
		
		bHit = true;
	}
	
	if (IsValid(LoopingSoundComponent))
	{
		LoopingSoundComponent->Stop();
	}
	// else{UKismetSystemLibrary::PrintString(this, FString(TEXT("Destroyed*******")), true, true, FLinearColor::Red, 3.f);}

	Super::Destroyed();
}

void AAuraProjectile::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// DamageEffectSpecHandle is only being set on server in UAuraProjectileSpell::SpawnProjectile
	// DamageEffectSpecHandle should be null on clients
	if (DamageEffectSpecHandle.IsValid())
	{
		AActor* EffectCauser = DamageEffectSpecHandle.Data->GetContext().GetEffectCauser();
		if (EffectCauser == OtherActor)
		{
			return;
		}
		if (! UAuraAbilitySystemLibrary::IsNotFriend(EffectCauser, OtherActor))
		{
			return;
		}
	}
	
	if (!DamageEffectSpecHandle.IsValid())
	{
		check(!HasAuthority());

		// Case: Enemy
		AActor* owner = GetOwner();
		if (owner == OtherActor)
		{
			return;
		}
		if (! UAuraAbilitySystemLibrary::IsNotFriend(owner, OtherActor))
		{
			return;
		}

		// Case: Aura
		const AAuraPlayerState* AuraPlayerState = GetOwner<AAuraPlayerState>();
		if (AuraPlayerState)
		{
			AActor* AvatarActor = (AuraPlayerState->GetAbilitySystemComponent()->GetAvatarActor());
			if (AvatarActor == OtherActor)
			{
				return;
			}
			if (! UAuraAbilitySystemLibrary::IsNotFriend(AvatarActor, OtherActor))
			{
				return;
			}
		}
	}

	if (!bHit)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation(), FRotator::ZeroRotator);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, GetActorLocation());

		if (IsValid(LoopingSoundComponent))
		{
			LoopingSoundComponent->Stop();
		}
		// else{UKismetSystemLibrary::PrintString(this, FString(TEXT("OnSphereOverlap*******")), true, true, FLinearColor::Red, 3.f);}
		
		bHit = true;
	}

	if (HasAuthority())
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageEffectSpecHandle.Data);
		}

		Destroy();
	}
}


