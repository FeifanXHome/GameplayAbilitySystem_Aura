// Copyright XXX

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AuraAbilityTypes.h"
#include "AuraProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;

UCLASS()
class AURA_API AAuraProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AAuraProjectile();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	FDamageEffectParams DamageEffectParams;

	UPROPERTY()
	TObjectPtr<USceneComponent> HomingTagetSceneComponent;

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void ApplyDamageWithoutKnockback(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void ApplyDamage(AActor* TargetActor,
		bool bOverrideKnockbackDirection = false, FVector KnockbackDirectionOverride = FVector::ZeroVector, float KnockbackMagnitude = 0.f,
		bool bOverrideDeathImpulse = false, FVector DeathImpulseDirectionOverride = FVector::ZeroVector, float DeathImpulseMagnitude = 0.f,
		bool bOverridePitch = true, float PitchOverride = 45.f,
		bool bInIsRadial = false, FVector RadialOrigin = FVector::ZeroVector, float RadialInnerRadius = 0.f, float RadialOuterRadius = 0.f
	);

	bool IsValidOverlap(AActor* OtherActor);

	UFUNCTION(BlueprintCallable)
	virtual void OnHit();

	bool bHit = false;

	UPROPERTY()
	TObjectPtr<UAudioComponent> LoopingSoundComponent;

private:

	UPROPERTY(EditAnywhere)
	float MinDistancePerFrame = 10.f;

	FVector LocationLastFrame;

	UPROPERTY(EditDefaultsOnly)
	float lifeSpan = 15.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> Sphere;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundBase> LoopingSound;
};
