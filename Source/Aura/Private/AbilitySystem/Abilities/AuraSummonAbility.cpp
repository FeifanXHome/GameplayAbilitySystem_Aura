// Copyright XXX


#include "AbilitySystem/Abilities/AuraSummonAbility.h"
#include "Kismet/KismetSystemLibrary.h"

TArray<FVector> UAuraSummonAbility::GetSpawnLocations()
{
    AActor* AvatarActor = GetAvatarActorFromActorInfo();

    const FVector Forward = AvatarActor->GetActorForwardVector();
    const FVector Location = AvatarActor->GetActorLocation();

	const float DeltaSpread = NumMinions == 0 ? 0 : SpawnSpread / NumMinions;
	const FVector LeftOfSpread_ = Forward.RotateAngleAxis(-SpawnSpread / 2.f, FVector::UpVector);

	TArray<FVector> SpawnLocations;
	for (int32 i=0; i<NumMinions; i++)
	{
		const float Angle = DeltaSpread * (i + 0.5f);
		const FVector Direction = LeftOfSpread_.RotateAngleAxis(Angle, FVector::UpVector);
		
		const FVector ChosenSpawnLocation = Location + Direction * FMath::FRandRange(MinSpawnDistance, MaxSpawnDistance);
		SpawnLocations.Add(ChosenSpawnLocation);

		DrawDebugSphere(GetWorld(), ChosenSpawnLocation, 6.f, 6, FColor::Red, false, 0.8f);

		UKismetSystemLibrary::DrawDebugArrow(AvatarActor, Location, Location + Direction * MaxSpawnDistance, 4, FLinearColor::MakeRandomColor(), 0.8f);
		DrawDebugSphere(GetWorld(), Location + Direction * MinSpawnDistance, 5.f, 6, FColor::Cyan, false, 0.8f);
		DrawDebugSphere(GetWorld(), Location + Direction * MaxSpawnDistance, 5.f, 6, FColor::Cyan, false, 0.8f);
	}

	const FVector RightOfSpread = Forward.RotateAngleAxis(SpawnSpread / 2.f + 2, FVector::UpVector);
	UKismetSystemLibrary::DrawDebugArrow(AvatarActor, Location, Location + RightOfSpread * MaxSpawnDistance, 4, FLinearColor::Green, 0.8f);

	DrawDebugSphere(GetWorld(), Location + RightOfSpread * MinSpawnDistance, 5.f, 6, FColor::Cyan, false, 0.8f);
	DrawDebugSphere(GetWorld(), Location + RightOfSpread * MaxSpawnDistance, 5.f, 6, FColor::Cyan, false, 0.8f);

	const FVector LeftOfSpread = Forward.RotateAngleAxis(-SpawnSpread / 2.f + 2, FVector::UpVector);
	UKismetSystemLibrary::DrawDebugArrow(AvatarActor, Location, Location + LeftOfSpread * MaxSpawnDistance, 4, FLinearColor::Green, 0.8f);

	DrawDebugSphere(GetWorld(), Location + LeftOfSpread * MinSpawnDistance, 5.f, 6, FColor::Cyan, false, 0.8f);
	DrawDebugSphere(GetWorld(), Location + LeftOfSpread * MaxSpawnDistance, 5.f, 6, FColor::Cyan, false, 0.8f);

    return SpawnLocations;
}
