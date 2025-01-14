// Copyright XXX


#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "UI/HUD/AuraHUD.h"
#include "Player/AuraPlayerState.h"
#include "UI/WidgetController/AuraWidgetController.h"
#include "Game/AuraGameModeBase.h"
#include "AbilitySystemComponent.h"
#include "AuraAbilityTypes.h"
#include "Interaction/CombatInterface.h"
#include "Aura/AuraLogChannels.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "AuraAbilityTypes.h"
#include "AuraGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"

int UAuraAbilitySystemLibrary::Debug(int flag, UObject* Object, FString String)
{
	return 0;
}

void UAuraAbilitySystemLibrary::GetAllChildWidgetsWidthClass2(const TArray<UWidget*>& ParentWidgets, TArray<UUserWidget*>& FoundWidgets, TSubclassOf<UUserWidget> WidgetClass)
{
	if (ParentWidgets.IsEmpty() || !WidgetClass) return;

	//Prevent possibility of an ever-growing array if user uses this in a loop
	FoundWidgets.Empty();

	for (auto Parent : ParentWidgets)
	{
		TArray<UUserWidget*> FoundWidgets_;
		GetAllChildWidgetsWidthClass(Parent, FoundWidgets_, WidgetClass);
		FoundWidgets.Append(FoundWidgets_);
	}
}

void UAuraAbilitySystemLibrary::GetAllChildWidgetsWidthClass(UWidget* ParentWidget, TArray<UUserWidget*>& FoundWidgets, TSubclassOf<UUserWidget> WidgetClass)
{
	if (!ParentWidget || !WidgetClass) return;

	//Prevent possibility of an ever-growing array if user uses this in a loop
	FoundWidgets.Empty();

	auto FunPredicate = [&FoundWidgets, &WidgetClass](UWidget* Widget)
		{
			if (Widget && Widget->GetClass()->IsChildOf(WidgetClass))
			{
				if (UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
				{
					FoundWidgets.Add(UserWidget);
				}
			}
		};

	if (const UPanelWidget* PanelParent = Cast<UPanelWidget>(ParentWidget))
	{
		UWidgetTree::ForWidgetAndChildren(ParentWidget, FunPredicate);
	}
	else if (const UUserWidget* UserWidget = Cast<UUserWidget>(ParentWidget))
	{
		if (UserWidget->WidgetTree)
		{
			UserWidget->WidgetTree->ForEachWidget(FunPredicate);
		}
	}
}

bool UAuraAbilitySystemLibrary::MakeWidgetControllerParams(const UObject* WorldContextObject, FWidgetControllerParams& OutWCParams, AAuraHUD*& OutAuraHUD)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		OutAuraHUD = Cast<AAuraHUD>(PC->GetHUD());
		if (OutAuraHUD)
		{
			AAuraPlayerState* PS = PC->GetPlayerState<AAuraPlayerState>();
			check(PS);

			UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
			UAttributeSet* AS = PS->GetAttributeSet();

			OutWCParams = FWidgetControllerParams(PC, PS, ASC, AS);
			return true;
		}
	}
	return false;
}

UOverlayWidgetController* UAuraAbilitySystemLibrary::GetOverlayWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AAuraHUD* AuraHUD = nullptr;
	if (MakeWidgetControllerParams(WorldContextObject, WCParams, AuraHUD))
	{
		return AuraHUD->GetOverlayWidgetController(WCParams);
	}
	return nullptr;
}

UAttributeMenuWidgetController* UAuraAbilitySystemLibrary::GetAttributeMenuWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AAuraHUD* AuraHUD = nullptr;
	if (MakeWidgetControllerParams(WorldContextObject, WCParams, AuraHUD))
	{
		return AuraHUD->GetAttributeMenuWidgetController(WCParams);
	}
	return nullptr;
}

USpellMenuWidgetController* UAuraAbilitySystemLibrary::GetSpellMenuWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AAuraHUD* AuraHUD = nullptr;
	if (MakeWidgetControllerParams(WorldContextObject, WCParams, AuraHUD))
	{
		return AuraHUD->GetSpellMenuWidgetController(WCParams);
	}
	return nullptr;
}

void UAuraAbilitySystemLibrary::InitializeDefaultAttributes(
	const UObject* WorldContextObject, ECharacterClass CharacterClass, float Level, UAbilitySystemComponent* ASC)
{
	UCharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	if (CharacterClassInfo == nullptr) return;

	FCharacterClassDefaultInfo ClassDefaultInfo = CharacterClassInfo->GetClassDefaultInfo(CharacterClass);

	AActor* AvatarActor = ASC->GetAvatarActor();

	FGameplayEffectContextHandle PrimaryAttributesContextHandle = ASC->MakeEffectContext();
	PrimaryAttributesContextHandle.AddSourceObject(AvatarActor);
	const FGameplayEffectSpecHandle PrimaryAttributesSpecHandle = ASC->MakeOutgoingSpec(ClassDefaultInfo.PrimaryAttributes, Level, PrimaryAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*PrimaryAttributesSpecHandle.Data);

	FGameplayEffectContextHandle SecondaryAttributesContextHandle = ASC->MakeEffectContext();
	SecondaryAttributesContextHandle.AddSourceObject(AvatarActor);
	const FGameplayEffectSpecHandle SecondaryAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->SecondaryAttributes, Level, SecondaryAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*SecondaryAttributesSpecHandle.Data);

	FGameplayEffectContextHandle VitalAttributesContextHandle = ASC->MakeEffectContext();
	VitalAttributesContextHandle.AddSourceObject(AvatarActor);
	const FGameplayEffectSpecHandle VitalAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->VitalAttributes, Level, VitalAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*VitalAttributesSpecHandle.Data);
}

void UAuraAbilitySystemLibrary::GiveStartupAbilities(const UObject* WorldContextObject, UAbilitySystemComponent* ASC, ECharacterClass CharacterClass)
{
	UCharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	if (CharacterClassInfo == nullptr) return;

	for (TSubclassOf<UGameplayAbility> AbilityClass : CharacterClassInfo->CommonAbilities)
	{
		FGameplayAbilitySpec AbilitySpec(AbilityClass, 1);
		ASC->GiveAbility(AbilitySpec);
	}

	const FCharacterClassDefaultInfo& DefaultInfo = CharacterClassInfo->GetClassDefaultInfo(CharacterClass);
	for (TSubclassOf<UGameplayAbility> AbilityClass : DefaultInfo.StartupAbilities)
	{
		if (ASC->GetAvatarActor()->Implements<UCombatInterface>())
		{
			int32 Level = ICombatInterface::Execute_GetPlayerLevel(ASC->GetAvatarActor());
			FGameplayAbilitySpec AbilitySpec(AbilityClass, Level);
			ASC->GiveAbility(AbilitySpec);
		}
	}
}

int32 UAuraAbilitySystemLibrary::GetXPRewardForClassAndLevel(const UObject* WorldContextObject, ECharacterClass CharacterClass, int32 CharacterLevel)
{
	UCharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	if (CharacterClassInfo == nullptr) 0;

	const FCharacterClassDefaultInfo& ClassDefaultInfo = CharacterClassInfo->GetClassDefaultInfo(CharacterClass);
	const float XPReward = ClassDefaultInfo.XPReward.GetValueAtLevel(CharacterLevel);

	return static_cast<int32>(XPReward);
}

UCharacterClassInfo* UAuraAbilitySystemLibrary::GetCharacterClassInfo(const UObject* WorldContextObject)
{
	const AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (AuraGameMode == nullptr) return nullptr;
	return AuraGameMode->CharacterClassInfo;
}

UAbilityInfo* UAuraAbilitySystemLibrary::GetAbilityInfo(const UObject* WorldContextObject)
{
	const AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (AuraGameMode == nullptr) return nullptr;
	return AuraGameMode->AbilityInfo;
}

bool UAuraAbilitySystemLibrary::IsBlockedHit(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->IsBlockedHit();
	}
	return false;
}

bool UAuraAbilitySystemLibrary::IsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->IsCriticalHit();
	}
	return false;
}

bool UAuraAbilitySystemLibrary::IsSuccessfulDebuff(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->IsSuccessfulDebuff();
	}
	return false;
}

float UAuraAbilitySystemLibrary::GetDebuffDamage(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDebuffDamage();
	}
	return 0.0f;
}

float UAuraAbilitySystemLibrary::GetDebuffDuration(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDebuffDuration();
	}
	return 0.0f;
}

float UAuraAbilitySystemLibrary::GetDebuffFrequency(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDebuffFrequency();
	}
	return 0.0f;
}

FGameplayTag UAuraAbilitySystemLibrary::GetDamageType(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDamageType();
	}
	return FGameplayTag();
}

FVector UAuraAbilitySystemLibrary::GetDeathImpulse(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDeathImpulse();
	}
	return FVector::ZeroVector;
}

FVector UAuraAbilitySystemLibrary::GetKnockbackForce(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetKnockbackForce();
	}
	return FVector::ZeroVector;
}

void UAuraAbilitySystemLibrary::SetIsBlockedHit(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, bool bInIsBlockedHit)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetIsBlockedHit(bInIsBlockedHit);
	}
}

void UAuraAbilitySystemLibrary::SetIsCriticalHit(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, bool bInIsCriticalHit)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetIsCriticalHit(bInIsCriticalHit);
	}
}

void UAuraAbilitySystemLibrary::SetIsSuccessfulDebuff(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, bool IsInDebuff)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetIsSuccessfulDebuff(IsInDebuff);
	}
}

void UAuraAbilitySystemLibrary::SetDebuffDamage(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, float InDamage)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDebuffDamage(InDamage);
	}
}

void UAuraAbilitySystemLibrary::SetDebuffDuration(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, float InDuration)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDebuffDuration(InDuration);
	}
}

void UAuraAbilitySystemLibrary::SetDebuffFrequency(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, float InFrequency)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDebuffFrequency(InFrequency);
	}
}

void UAuraAbilitySystemLibrary::SetDamageType(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, FGameplayTag InDamageType)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDamageType(InDamageType);
	}
}

void UAuraAbilitySystemLibrary::SetDeathImpulse(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, FVector InDeathImpulse)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDeathImpulse(InDeathImpulse);
	}
}

void UAuraAbilitySystemLibrary::SetKnockbackForce(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, FVector InForce)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = dynamic_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetKnockbackForce(InForce);
	}
}

FGameplayEffectSpecHandle UAuraAbilitySystemLibrary::GetAllDataTagsFromSetByCallerMagnitudes(FGameplayEffectSpecHandle SpecHandle, TArray<FGameplayTag>& OutDataTags)
{
	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
	if (Spec)
	{
		for (int32 ModIdx = 0; ModIdx < Spec->Def->Modifiers.Num(); ++ModIdx)
		{
			const FGameplayModifierInfo& ModDef = Spec->Def->Modifiers[ModIdx];

			if (ModDef.ModifierMagnitude.GetMagnitudeCalculationType() == EGameplayEffectMagnitudeCalculation::SetByCaller)
			{
				const FSetByCallerFloat& SetByCallerFloat = ModDef.ModifierMagnitude.GetSetByCallerFloat();
				OutDataTags.Add(SetByCallerFloat.DataTag);
			}
		}
	}
	else
	{
		UE_LOG(LogAura, Warning, TEXT("UAuraAbilitySystemLibrary::GetAllDataTagsFromSetByCallerMagnitudes called with invalid SpecHandle"));
	}

	return SpecHandle;
}

void UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(const UObject* WorldContextObject, 
	TArray<AActor*>& OutOverlappingActors, const TArray<AActor*>& ActorsToIgnore, float Radius, const FVector& SphereOrigin)
{
	FCollisionQueryParams SphereParams;
	SphereParams.AddIgnoredActors(ActorsToIgnore);

	// query scene to see what we hit
	TArray<FOverlapResult> Overlaps;

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr) return;

	World->OverlapMultiByObjectType(
		Overlaps,
		SphereOrigin,
		FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects),
		FCollisionShape::MakeSphere(Radius),
		SphereParams
	);
	
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* const OverlapActor = Overlap.GetActor();// Overlap.OverlapObjectHandle.FetchActor();
		if (OverlapActor->Implements<UCombatInterface>() && !ICombatInterface::Execute_IsDead(OverlapActor))
		{
			OutOverlappingActors.AddUnique(OverlapActor);
		}
	}
}

bool UAuraAbilitySystemLibrary::IsNotFriend(AActor* FirstActor, AActor* SecondActor)
{
	const FName PlayerTag(TEXT("Player"));
	const FName EnemyTag (TEXT("Enemy"));
	const bool bBothArePlayers = FirstActor->ActorHasTag(PlayerTag) && SecondActor->ActorHasTag(PlayerTag);
	const bool bBothAreEnemies = FirstActor->ActorHasTag(EnemyTag)  && SecondActor->ActorHasTag(EnemyTag);
	const bool bFriends = bBothArePlayers || bBothAreEnemies;
	return !bFriends;
}

TArray<FRotator> UAuraAbilitySystemLibrary::EvenlySpacedRotators(float Spread, int32 Count, const FVector Forward, const FVector Axis)
{
	TArray<FRotator> Rotators;

	TArray<FVector> Vectors = EvenlyRotatedVectors(Spread, Count, Forward, Axis);
	for (const FVector& Vector : Vectors)
	{
		Rotators.Add(Vector.Rotation());
	}

	return Rotators;
}

TArray<FVector> UAuraAbilitySystemLibrary::EvenlyRotatedVectors(float Spread, int32 Count, const FVector Forward, const FVector Axis)
{
	TArray<FVector> Vectors;

	Count = FMath::Max(Count, 1);
	const int32 IsOnlyOne = (Count == 1);
	const int32 IsNotOnlyOne = ~IsOnlyOne & 1;

	const float DeltaSpread = Spread / (Count - 1 * IsNotOnlyOne);
	const FVector LeftOfSpread = Forward.RotateAngleAxis(-Spread / 2.f, Axis);
	const float Offset = (DeltaSpread / 2) * (IsOnlyOne);

	for (int32 i = 0; i < Count; i++)
	{
		const float Angle = DeltaSpread * (i) + Offset;
		const FVector Direction = LeftOfSpread.RotateAngleAxis(Angle, Axis);
		Vectors.Add(Direction);
	}

	/*if (Count <= 1)
	{
		Vectors.Add(Forward.GetSafeNormal());
	}
	else
	{
		const float DeltaSpread = Spread / (Count - 1);
		const FVector LeftOfSpread = Forward.RotateAngleAxis(-Spread / 2.f, Axis);

		for (int32 i = 0; i < Count; i++)
		{
			const float Angle = DeltaSpread * (i);
			const FVector Direction = LeftOfSpread.RotateAngleAxis(Angle, Axis);
			Vectors.Add(Direction);
		}
	}*/

	return Vectors;
}

FGameplayEffectContextHandle UAuraAbilitySystemLibrary::ApplyDamageEffect(const FDamageEffectParams& DamageEffectParams)
{
	FGameplayEffectContextHandle EffectContextHandle;

	if (DamageEffectParams.TargetAbilitySystemComponent == nullptr) return EffectContextHandle;
	if (DamageEffectParams.SourceAbilitySystemComponent == nullptr) return EffectContextHandle;
	if (DamageEffectParams.DamageGameplayEffectClass    == nullptr) return EffectContextHandle;

	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	const AActor* SourceAvatarActor = DamageEffectParams.SourceAbilitySystemComponent->GetAvatarActor();

	EffectContextHandle = DamageEffectParams.SourceAbilitySystemComponent->MakeEffectContext();
	EffectContextHandle.AddSourceObject(SourceAvatarActor);

	const FGameplayEffectSpecHandle SpecHandle =
		DamageEffectParams.SourceAbilitySystemComponent->MakeOutgoingSpec(
			DamageEffectParams.DamageGameplayEffectClass,
			DamageEffectParams.AbilityLevel,
			EffectContextHandle
		);

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, DamageEffectParams.DamageType,	  DamageEffectParams.BaseDamage);

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Debuff_Info_Chance,	  DamageEffectParams.DebuffChance);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Debuff_Info_Damage,	  DamageEffectParams.DebuffDamage);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Debuff_Info_Duration,  DamageEffectParams.DebuffDuration);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Debuff_Info_Frequency, DamageEffectParams.DebuffFrequency);

	UAuraAbilitySystemLibrary::SetIsSuccessfulDebuff(EffectContextHandle, false);
	UAuraAbilitySystemLibrary::SetDebuffDamage		(EffectContextHandle, DamageEffectParams.DebuffDamage);
	UAuraAbilitySystemLibrary::SetDebuffDuration	(EffectContextHandle, DamageEffectParams.DebuffDuration);
	UAuraAbilitySystemLibrary::SetDebuffFrequency	(EffectContextHandle, DamageEffectParams.DebuffFrequency);
	UAuraAbilitySystemLibrary::SetDamageType		(EffectContextHandle, DamageEffectParams.DamageType);
	UAuraAbilitySystemLibrary::SetDeathImpulse		(EffectContextHandle, DamageEffectParams.DeathImpulse);
	UAuraAbilitySystemLibrary::SetKnockbackForce	(EffectContextHandle, DamageEffectParams.KnockbackForce);

	DamageEffectParams.TargetAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);

	return EffectContextHandle;
}
