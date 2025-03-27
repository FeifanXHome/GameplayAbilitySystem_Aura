// Copyright XXX


#include "Actor/AuraEnemySpawnVolume.h"
#include "Components/BoxComponent.h"
#include "Interaction/PlayerInterface.h"
#include "Actor/AuraEnemySpawnPoint.h"

AAuraEnemySpawnVolume::AAuraEnemySpawnVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Boxx"));
	//Box->bHiddenInGame = false;
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	SetRootComponent(Box);
}

void AAuraEnemySpawnVolume::LoadActor_Implementation()
{
	if (bReached)
	{
		Destroy();
	}
}

void AAuraEnemySpawnVolume::BeginPlay()
{
	Super::BeginPlay();
	
	Box->OnComponentBeginOverlap.AddDynamic(this, &AAuraEnemySpawnVolume::OnBoxOverlap);
}

void AAuraEnemySpawnVolume::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor->Implements<UPlayerInterface>()) return;

	bReached = true;

	for (AAuraEnemySpawnPoint* Point : SpawnPoints)
	{
		if (IsValid(Point))
		{
			Point->SpawnEnemy();
		}
	}

	Box->SetGenerateOverlapEvents(false);
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

