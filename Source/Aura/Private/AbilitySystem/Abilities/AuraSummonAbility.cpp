// Copyright XXX


#include "AbilitySystem/Abilities/AuraSummonAbility.h"
#include "Kismet/KismetSystemLibrary.h"


#define DRAW_TIME 0.8f
TArray<FVector> UAuraSummonAbility::GetSpawnLocations(bool IsShowDebugShapes)
{
    AActor* AvatarActor = GetAvatarActorFromActorInfo();

    const FVector Forward = AvatarActor->GetActorForwardVector();
    const FVector Location = AvatarActor->GetActorLocation();

	const float DeltaSpread = NumMinions == 0 ? 0 : SpawnSpread / NumMinions;
	const FVector LeftOfSpread_ = Forward.RotateAngleAxis(-SpawnSpread / 2.f, FVector::UpVector);

	TArray<FVector> SpawnLocations;
	for (int32 i=0; i<NumMinions; i++)
	{
		FColor SpawnColor = FColor::Red;

		const float Angle = DeltaSpread * (i + 0.5f);
		const FVector Direction = LeftOfSpread_.RotateAngleAxis(Angle, FVector::UpVector);
		
		FVector ChosenSpawnLocation = Location + Direction * FMath::FRandRange(MinSpawnDistance, MaxSpawnDistance);

		const FVector StartPoint = ChosenSpawnLocation + FVector(0.f, 0.f, 400.f);
		const FVector EndPoint   = ChosenSpawnLocation - FVector(0.f, 0.f, 400.f);

		const TArray<AActor*> ActorsToIgnore;
		FHitResult HitResult;
		//GetWorld()->LineTraceSingleByChannel(HitResult, StartPoint, EndPoint, ECC_Visibility);
		UKismetSystemLibrary::LineTraceSingle(
			GetOuter(), StartPoint, EndPoint, ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, 
			IsShowDebugShapes ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None, 
			HitResult, true, FLinearColor::Red, FLinearColor::Green, DRAW_TIME
		);

		if (HitResult.bBlockingHit)
		{
			ChosenSpawnLocation = HitResult.ImpactPoint;
			SpawnLocations.Add(ChosenSpawnLocation);

			SpawnColor = FColor::Green;
		}

		if (IsShowDebugShapes)
		{
			DrawDebugSphere(GetWorld(), ChosenSpawnLocation, 6.f, 6, SpawnColor, false, DRAW_TIME);

			UKismetSystemLibrary::DrawDebugArrow(AvatarActor, Location, Location + Direction * MaxSpawnDistance, 4, FLinearColor::MakeRandomColor(), DRAW_TIME);
			DrawDebugSphere(GetWorld(), Location + Direction * MinSpawnDistance, 5.f, 6, FColor::Cyan, false, DRAW_TIME);
			DrawDebugSphere(GetWorld(), Location + Direction * MaxSpawnDistance, 5.f, 6, FColor::Cyan, false, DRAW_TIME);
		}
	}

	if (IsShowDebugShapes)
	{
		const FVector RightOfSpread = Forward.RotateAngleAxis(SpawnSpread / 2.f + 2, FVector::UpVector);
		UKismetSystemLibrary::DrawDebugArrow(AvatarActor, Location, Location + RightOfSpread * MaxSpawnDistance, 4, FLinearColor::Green, DRAW_TIME);

		DrawDebugSphere(GetWorld(), Location + RightOfSpread * MinSpawnDistance, 5.f, 6, FColor::Cyan, false, DRAW_TIME);
		DrawDebugSphere(GetWorld(), Location + RightOfSpread * MaxSpawnDistance, 5.f, 6, FColor::Cyan, false, DRAW_TIME);

		const FVector LeftOfSpread = Forward.RotateAngleAxis(-SpawnSpread / 2.f + 2, FVector::UpVector);
		UKismetSystemLibrary::DrawDebugArrow(AvatarActor, Location, Location + LeftOfSpread * MaxSpawnDistance, 4, FLinearColor::Green, DRAW_TIME);

		DrawDebugSphere(GetWorld(), Location + LeftOfSpread * MinSpawnDistance, 5.f, 6, FColor::Cyan, false, DRAW_TIME);
		DrawDebugSphere(GetWorld(), Location + LeftOfSpread * MaxSpawnDistance, 5.f, 6, FColor::Cyan, false, DRAW_TIME);
	}

    return SpawnLocations;
}
