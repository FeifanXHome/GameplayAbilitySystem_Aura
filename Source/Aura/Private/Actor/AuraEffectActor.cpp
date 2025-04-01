// Copyright XXX


#include "Actor/AuraEffectActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/AuraCharacterBase.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
AAuraEffectActor::AAuraEffectActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("SceneRoott"));
}

void AAuraEffectActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RunningTime += DeltaSeconds;
	const float SinePeriod = 2 * PI / SinePeriodConstant;
	if (RunningTime > SinePeriod)
	{
		RunningTime = 0;
	}

	ItemMovement(DeltaSeconds);
}

void AAuraEffectActor::ItemMovement(float DeltaSeconds)
{
	if (bRotates)
	{
		const FRotator DeltaRotation(0.f, DeltaSeconds * RotationRate, 0.f);
		CalculatedRotation = UKismetMathLibrary::ComposeRotators(CalculatedRotation, DeltaRotation);
	}
	if (bSinusoidalMovement)
	{
		const float Sine = SineAmplitude * FMath::Sin(RunningTime * SinePeriodConstant);
		CalculatedLocation = InitialLocation + FVector(0.f, 0.f, Sine);
	}
}

// Called when the game starts or when spawned
void AAuraEffectActor::BeginPlay()
{
	Super::BeginPlay();

	const float SinePeriod = 2 * PI / SinePeriodConstant;
	RunningTime = FMath::FRandRange(0, SinePeriod);

	InitialLocation = GetActorLocation();
	CalculatedLocation = InitialLocation;
	CalculatedRotation = GetActorRotation();
}

void AAuraEffectActor::StartSinusoidalMovement()
{
	bSinusoidalMovement = true;
	InitialLocation = GetActorLocation();
	CalculatedLocation = InitialLocation;
}

void AAuraEffectActor::StartRotation()
{
	bRotates = true;
	CalculatedRotation = GetActorRotation();
}

void AAuraEffectActor::ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	if (!TargetActor->IsA(AAuraCharacterBase::StaticClass())) return;
	if (TargetActor->ActorHasTag(TEXT("Enemy")) && !bApplyEffectsToEnemies) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	/**
	* BP_SlingshotRock 
	* Option 1: Remove the simplified collision of the static mesh for RockMesh
	*			- The overlapping event will happen only if the static mesh has the simplified collision.
	* Option 2: Set Generate Overlap Events to False for RockMesh
	*/ 
	check(TargetASC); 
	if (TargetASC == nullptr) return;

	check(GameplayEffectClass);
	FGameplayEffectContextHandle EffectContextHandle = TargetASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);
	const FGameplayEffectSpecHandle EffectSpecHandle = TargetASC->MakeOutgoingSpec(GameplayEffectClass, ActorLevel, EffectContextHandle);
	const FActiveGameplayEffectHandle ActiveEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data);

	EGameplayEffectDurationType DurationPolicy = EffectSpecHandle.Data->Def->DurationPolicy;
	bool bIsInfinite = DurationPolicy == EGameplayEffectDurationType::Infinite;
	if (InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap || InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		if (InfiniteEffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap)
		{
			check(bIsInfinite);
			if (bIsInfinite)
			{
				//check(!ActiveEffectHandles.Contains(ActiveEffectHandle));
				TArray<UAbilitySystemComponent*>& array = ActiveEffectHandles.FindOrAdd(ActiveEffectHandle);
				check(!array.Contains(TargetASC));
				array.Add(TargetASC);
			}
		}
	}

	if (bDestroyOnEffectApplication)
	{
		if (!bIsInfinite)
		{
			Destroy();
		}
	}
	else
	{
		check(bIsInfinite && "Instant/HasDuration Effect need to set bDestroyOnEffectApplication to True");
	}
}

void AAuraEffectActor::OnOverlap(AActor* TargetActor)
{
	if (TargetActor->ActorHasTag(TEXT("Enemy")) && !bApplyEffectsToEnemies) return;

	if (InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
	}
	if (DurationEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
	}
	if (InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
	}
}

void AAuraEffectActor::OnEndOverlap(AActor* TargetActor)
{
	if (TargetActor->ActorHasTag(TEXT("Enemy")) && !bApplyEffectsToEnemies) return;

	if (InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
	}
	if (DurationEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
	}
	if (InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
	}
	if (InfiniteEffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap)
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (IsValid(TargetActor))
		{
			TArray<FActiveGameplayEffectHandle> HandlesToRemove;
			for (TTuple<FActiveGameplayEffectHandle, TArray<UAbilitySystemComponent*>>& HandlePair : ActiveEffectHandles)
			{
				while (HandlePair.Value.Contains(TargetASC))
				{
					TargetASC->RemoveActiveGameplayEffect(HandlePair.Key, 1);
					HandlePair.Value.Remove(TargetASC);
				}
				if (HandlePair.Value.IsEmpty())
				{
					HandlesToRemove.Add(HandlePair.Key);
				}
			}
			for (FActiveGameplayEffectHandle& Handle : HandlesToRemove)
			{
				ActiveEffectHandles.FindAndRemoveChecked(Handle);
			}
		}
	}
}

