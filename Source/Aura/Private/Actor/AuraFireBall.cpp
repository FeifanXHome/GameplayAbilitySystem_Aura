// Copyright XXX


#include "Actor/AuraFireBall.h"
#include "Components/AudioComponent.h"
#include "GameplayCueManager.h"
#include "AuraGameplayTags.h"

AAuraFireBall::AAuraFireBall()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AAuraFireBall::BeginPlay()
{
	Super::BeginPlay();
	StartOutgoingTimeline();
}

void AAuraFireBall::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValidOverlap(OtherActor))
	{
		return;
	}

	if (HasAuthority())
	{
		ApplyDamageWithoutKnockback(OtherActor);
	}
}

void AAuraFireBall::OnHit()
{
	if (GetOwner())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = GetActorLocation();
		UGameplayCueManager::ExecuteGameplayCue_NonReplicated(GetOwner(), FAuraGameplayTags::Get().GameplayCue_FireBlast, CueParams);
	}

	if (IsValid(LoopingSoundComponent))
	{
		LoopingSoundComponent->Stop();
		LoopingSoundComponent->DestroyComponent();
	}

	bHit = true;
}
