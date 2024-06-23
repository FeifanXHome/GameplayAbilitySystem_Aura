// Copyright XXX


#include "Character/AuraCharacterBase.h"

// Sets default values
AAuraCharacterBase::AAuraCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// FName   -> "a regular string literal"
	// FString -> TEXT("a regular string literal")
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weaponn"));
	// Weapon->SetupAttachment(GetRootComponent());
	// FAttachmentTransformRules TransformRules(EAttachmentRule::SnapToTarget, true);
	// Weapon->AttachToComponent(InParent, TransformRules, InSocketName);
	Weapon->SetupAttachment(GetMesh(), FName("WeaponHandSocket"));
}

// Called when the game starts or when spawned
void AAuraCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}


