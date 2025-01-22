// Copyright XXX


#include "AbilitySystem/Abilities/AuraBeamSpell.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"

void UAuraBeamSpell::StoreMouseDataInfo(const FHitResult& HitResult)
{
	if (HitResult.bBlockingHit)
	{
		MouseHitLocation = HitResult.ImpactPoint;
		MouseHitActor = HitResult.GetActor();
	}
	else
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
}

void UAuraBeamSpell::StoreOwnerVariables()
{
	if (CurrentActorInfo)
	{
		OwnerPlayerController = CurrentActorInfo->PlayerController.Get();
		OwnerCharacter = Cast<ACharacter>(CurrentActorInfo->AvatarActor);
	}
}

void UAuraBeamSpell::TraceFirstTarget(FHitResult& OutHitResult, bool& OutBlockingHit, const FVector& CombatSocketLocation, const FVector& BeamTargetLocation, float SphereRadius, bool IsShowDebugShapes)
{
	check(OwnerCharacter);

	const FVector StartPoint = CombatSocketLocation;
	const FVector EndPoint = BeamTargetLocation;

	const TArray<AActor*> ActorsToIgnore;
	FHitResult HitResult;
	
	UKismetSystemLibrary::SphereTraceSingle(
		OwnerCharacter, 
		StartPoint, 
		EndPoint, 
		SphereRadius,
		ETraceTypeQuery::TraceTypeQuery1, // TraceTypeQuery1 -> ECC_Visibility
		false, 
		ActorsToIgnore, 
		IsShowDebugShapes ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None, 
		HitResult, 
		true, 
		FLinearColor::Red, 
		FLinearColor::Green, 
		0.8f
	);

	OutHitResult = HitResult;
	OutBlockingHit = OutHitResult.bBlockingHit;
}
