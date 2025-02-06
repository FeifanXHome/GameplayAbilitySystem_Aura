// Copyright XXX


#include "Character/AuraCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Aura/Aura.h"
#include "AuraGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystem/Debuff/DebuffNiagaraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/Passive/PassiveNiagaraComponent.h"

// Sets default values
AAuraCharacterBase::AAuraCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BurnDebuffComponent = CreateDefaultSubobject<UDebuffNiagaraComponent>(TEXT("BurnDebuffComponentt"));
	BurnDebuffComponent->SetupAttachment(GetRootComponent());
	BurnDebuffComponent->DebuffTag = FAuraGameplayTags::Get().Debuff_Type_FireBurn;

	StunDebuffComponent = CreateDefaultSubobject<UDebuffNiagaraComponent>(TEXT("StunDebuffComponentt"));
	StunDebuffComponent->SetupAttachment(GetRootComponent());
	StunDebuffComponent->DebuffTag = FAuraGameplayTags::Get().Debuff_Type_LightningStun;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);

	// FName   -> "a regular string literal"
	// FString -> TEXT("a regular string literal")
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weaponn"));
	// Weapon->SetupAttachment(GetRootComponent());
	// FAttachmentTransformRules TransformRules(EAttachmentRule::SnapToTarget, true);
	// Weapon->AttachToComponent(InParent, TransformRules, InSocketName);
	Weapon->SetupAttachment(GetMesh(), FName("WeaponHandSocket"));
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EffectAttachComponent = CreateDefaultSubobject<USceneComponent>((TEXT("EffectAttachPointt")));
	EffectAttachComponent->SetupAttachment(GetRootComponent());
	HaloOfProtectionNiagaraComponent = CreateDefaultSubobject<UPassiveNiagaraComponent>((TEXT("HaloOfProtectionNiagaraComponentt")));
	HaloOfProtectionNiagaraComponent->PassiveSpellTag = FAuraGameplayTags::Get().Abilities_Passive_HaloOfProtection;
	HaloOfProtectionNiagaraComponent->SetupAttachment(EffectAttachComponent);
	LifeSiphonNiagaraComponent = CreateDefaultSubobject<UPassiveNiagaraComponent>((TEXT("LifeSiphonNiagaraComponentt")));
	LifeSiphonNiagaraComponent->PassiveSpellTag = FAuraGameplayTags::Get().Abilities_Passive_LifeSiphon;
	LifeSiphonNiagaraComponent->SetupAttachment(EffectAttachComponent);
	ManaSiphonNiagaraComponent = CreateDefaultSubobject<UPassiveNiagaraComponent>((TEXT("ManaSiphonNiagaraComponentt")));
	ManaSiphonNiagaraComponent->PassiveSpellTag = FAuraGameplayTags::Get().Abilities_Passive_ManaSiphon;
	ManaSiphonNiagaraComponent->SetupAttachment(EffectAttachComponent);
}

void AAuraCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	EffectAttachComponent->SetWorldRotation(FRotator::ZeroRotator);
}

void AAuraCharacterBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAuraCharacterBase, bIsStunned);
	DOREPLIFETIME(AAuraCharacterBase, bIsBurned);
	DOREPLIFETIME(AAuraCharacterBase, bIsBeingShocked);
}

UAbilitySystemComponent* AAuraCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAnimMontage* AAuraCharacterBase::GetHitReactMontage_Implementation()
{
	return HitReactMontage;
}

// called specifically on the server
void AAuraCharacterBase::Die(const FVector& DeathImpulse)
{
	// The detachment is something that will automatically be a replicated action
	// So if we detach on the server, we don't have to detach on clients.
	Weapon->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));

	// There are a number of things I want to do that we can't just do on the server
	// We have to do on the server and clients
	MulticastHandleDeath(DeathImpulse);
}

// called on all machines, client and server
void AAuraCharacterBase::MulticastHandleDeath_Implementation(const FVector& DeathImpulse)
{
	UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation(), GetActorRotation());

	Weapon->SetSimulatePhysics(true);
	Weapon->SetEnableGravity(true);
	Weapon->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	Weapon->AddImpulse(DeathImpulse * 0.1, NAME_None, true);

	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetEnableGravity(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	GetMesh()->AddImpulse(DeathImpulse, NAME_None, true);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	Dissolve();

	// We don't have to make this a replicated variable.
	// MulticastHandleDeath will be called on clients and server.
	bDead = true;

	OnDeathDelegate.Broadcast(this);
}

void AAuraCharacterBase::RegisterGameplayTagEvents()
{
	AbilitySystemComponent->RegisterGameplayTagEvent(
		FAuraGameplayTags::Get().Debuff_Type_LightningStun,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &AAuraCharacterBase::OnGameplayTagChanged);

	AbilitySystemComponent->RegisterGameplayTagEvent(
		FAuraGameplayTags::Get().Debuff_Type_FireBurn,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &AAuraCharacterBase::OnGameplayTagChanged);
}

void AAuraCharacterBase::OnGameplayTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	if (CallbackTag.MatchesTagExact(GameplayTags.Debuff_Type_LightningStun))
	{
		bIsStunned = NewCount > 0;
		GetCharacterMovement()->MaxWalkSpeed = bIsStunned ? 0.f : BaseWalkSpeed;
	}
	else if (CallbackTag.MatchesTagExact(GameplayTags.Debuff_Type_FireBurn))
	{
		bIsBurned = NewCount > 0;
	}
}

// OnRep_Stunned will only be called on clients when bIsStunned is replicated.
void AAuraCharacterBase::OnRep_Stunned(const bool& OldStunned)
{
	if (AbilitySystemComponent)
	{
		const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();

		FGameplayTagContainer BlockedTags;

		// Block All Input Motion from AAuraPlayerController
		BlockedTags.AddTag(GameplayTags.Player_Block_CursorTrace);
		BlockedTags.AddTag(GameplayTags.Player_Block_InputHeld);
		BlockedTags.AddTag(GameplayTags.Player_Block_InputPressed);
		BlockedTags.AddTag(GameplayTags.Player_Block_InputReleased);

		// Trigger UDebuffNiagaraComponent::DebuffTagChanged for StunDebuffComponent
		BlockedTags.AddTag(GameplayTags.Debuff_Type_LightningStun);

		if (bIsStunned)
		{
			AbilitySystemComponent->AddLooseGameplayTags(BlockedTags);
		}
		else
		{
			AbilitySystemComponent->RemoveLooseGameplayTags(BlockedTags);
		}
	}
}

void AAuraCharacterBase::OnRep_Burned(const bool& OldStunned)
{
	if (AbilitySystemComponent)
	{
		const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();

		FGameplayTagContainer BlockedTags;

		// Trigger UDebuffNiagaraComponent::DebuffTagChanged for BurnDebuffComponent
		BlockedTags.AddTag(GameplayTags.Debuff_Type_FireBurn);

		if (bIsBurned)
		{
			AbilitySystemComponent->AddLooseGameplayTags(BlockedTags);
		}
		else
		{
			AbilitySystemComponent->RemoveLooseGameplayTags(BlockedTags);
		}
	}
}

// Called when the game starts or when spawned
void AAuraCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}

FVector AAuraCharacterBase::GetCombatSocketLocation_Implementation(const FGameplayTag& MontageTag)
{
	const FAuraGameplayTags GameplayTags = FAuraGameplayTags::Get();
	if (MontageTag.MatchesTagExact(GameplayTags.CombatSocket_Weapon))
	{
		check(Weapon);
		checkf(Weapon->DoesSocketExist(WeaponTipSocketName), TEXT("[Weapon]:: [%s] does not exist on mesh [%s] of [%s] !"), *WeaponTipSocketName.ToString(), *Weapon->GetName(), *this->GetName())
		return Weapon->GetSocketLocation(WeaponTipSocketName);
	}
	if (MontageTag.MatchesTagExact(GameplayTags.CombatSocket_LeftHand))
	{
		checkf(GetMesh()->DoesSocketExist(LeftHandSocketName), TEXT("[LeftHand]:: [%s] does not exist on mesh [%s] of [%s] !"), *LeftHandSocketName.ToString(), *GetMesh()->GetName(), *this->GetName())
		return GetMesh()->GetSocketLocation(LeftHandSocketName);
	}
	if (MontageTag.MatchesTagExact(GameplayTags.CombatSocket_RightHand))
	{
		checkf(GetMesh()->DoesSocketExist(RightHandSocketName), TEXT("[RightHand]:: [%s] does not exist on mesh [%s] of [%s] !"), *RightHandSocketName.ToString(), *GetMesh()->GetName(), *this->GetName())
		return GetMesh()->GetSocketLocation(RightHandSocketName);
	}
	if (MontageTag.MatchesTagExact(GameplayTags.CombatSocket_Tail))
	{
		checkf(GetMesh()->DoesSocketExist(TailSocketName), TEXT("[Tail]:: [%s] does not exist on mesh [%s] of [%s] !"), *TailSocketName.ToString(), *GetMesh()->GetName(), *this->GetName())
		return GetMesh()->GetSocketLocation(TailSocketName);
	}
	checkf(false, TEXT("[GetCombatSocketLocation]:: Type [%s] does not exist !"), *MontageTag.ToString())
	return FVector();
}

bool AAuraCharacterBase::IsDead_Implementation() const
{
	return bDead;
}

AActor* AAuraCharacterBase::GetAvatar_Implementation()
{
	return this;
}

TArray<FTaggedMontage> AAuraCharacterBase::GetAttackMontages_Implementation()
{
	return AttackMontages;
}

UNiagaraSystem* AAuraCharacterBase::GetBloodEffect_Implementation()
{
	return BloodEffect;
}

FTaggedMontage AAuraCharacterBase::GetTaggedMontageByTag_Implementation(const FGameplayTag& MontageTag)
{
	for (const FTaggedMontage& TaggedMontage : AttackMontages)
	{
		if (TaggedMontage.MontageTag.MatchesTagExact(MontageTag))
		{
			return TaggedMontage;
		}
	}
	return FTaggedMontage();
}

int32 AAuraCharacterBase::GetMinionCount_Implementation()
{
	return MinionCount;
}

void AAuraCharacterBase::IncrementMinionCount_Implementation(int32 Amount)
{
	MinionCount += Amount;
}

ECharacterClass AAuraCharacterBase::GetCharacterClass_Implementation()
{
	return CharacterClass;
}

FOnASCRegisteredSignature& AAuraCharacterBase::GetOnASCRegisteredDelegate()
{
	return OnASCRegisteredDelegate;
}

FOnDeathSignature& AAuraCharacterBase::GetOnDeathDelegate()
{
	return OnDeathDelegate;
}

void AAuraCharacterBase::AddDeathDelegate_Implementation(const FDeathDynamicDelegate& Delegate)
{
	OnDeathDelegate.AddUnique(Delegate);
}

void AAuraCharacterBase::RemoveDeathDelegate_Implementation(const FDeathDynamicDelegate& Delegate)
{
	OnDeathDelegate.Remove(Delegate);
	//OnDeathDelegate.RemoveAll(Delegate.GetUObject());
}

bool AAuraCharacterBase::IsBeingShocked_Implementation() const
{
	return bIsBeingShocked;
}

void AAuraCharacterBase::SetIsBeingShocked_Implementation(bool bInShock)
{
	bIsBeingShocked = bInShock;
}

USkeletalMeshComponent* AAuraCharacterBase::GetWeapon_Implementation()
{
	return Weapon;
}

void AAuraCharacterBase::InitAbilityActorInfo()
{
}

void AAuraCharacterBase::ApplayEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level) const
{
	check(IsValid(GetAbilitySystemComponent()));
	check(GameplayEffectClass);

	FGameplayEffectContextHandle  ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	const FGameplayEffectSpecHandle  SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(GameplayEffectClass, Level, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, GetAbilitySystemComponent());
}

void AAuraCharacterBase::InitializeDefaultAttributes() const
{
	ApplayEffectToSelf(DefaultPrimaryAttributes, 1.f);
	ApplayEffectToSelf(DefaultSecondaryAttributes, 1.f);
	ApplayEffectToSelf(DefaultVitalAttributes, 1.f);
}

void AAuraCharacterBase::AddCharacherAbilities()
{
	if (!HasAuthority()) return;

	UAuraAbilitySystemComponent* AuraASC = CastChecked<UAuraAbilitySystemComponent>(AbilitySystemComponent);
	AuraASC->AddCharacherAbilities(StartupAbilities);
	AuraASC->AddCharacherPassiveAbilities(StartupPassiveAbilities);
}

void AAuraCharacterBase::Dissolve()
{
	if (IsValid(DissolveMaterialInstance))
	{
		UMaterialInstanceDynamic* DynamicMatInst = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicMatInst);
		StartDissolveTimeline(DynamicMatInst);
	}
	if (IsValid(WeaponDissolveMaterialInstance))
	{
		UMaterialInstanceDynamic* DynamicMatInst = UMaterialInstanceDynamic::Create(WeaponDissolveMaterialInstance, this);
		Weapon->SetMaterial(0, DynamicMatInst);
		StartWeaponDissolveTimeline(DynamicMatInst);
	}
}

