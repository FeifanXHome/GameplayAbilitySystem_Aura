// Copyright XXX


#include "AbilitySystem/Passive/PassiveNiagaraComponent.h"
#include "Interaction/CombatInterface.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AuraGameplayTags.h"

UPassiveNiagaraComponent::UPassiveNiagaraComponent()
{
	bAutoActivate = false;
}

void UPassiveNiagaraComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	ICombatInterface* CombatInterface = Cast<ICombatInterface>(Owner);
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);

	if (UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(ASC))
	{
		AuraASC->ActivatePassiveEffect.AddUObject(this, &UPassiveNiagaraComponent::OnPassiveActivate);
		ActivateIfEquiped(AuraASC);
	}
	else if (CombatInterface)
	{
		CombatInterface->GetOnASCRegisteredDelegate().AddWeakLambda(
			this,
			[this](UAbilitySystemComponent* InASC)
			{
				check(InASC);
				if (UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(InASC))
				{
					AuraASC->ActivatePassiveEffect.AddUObject(this, &UPassiveNiagaraComponent::OnPassiveActivate);
					ActivateIfEquiped(AuraASC);
				}
			}
		);
	}

	if (CombatInterface)
	{
		CombatInterface->GetOnDeathDelegate().AddDynamic(this, &UPassiveNiagaraComponent::OnOwnerDeath);
	}
}

void UPassiveNiagaraComponent::OnPassiveActivate(const FGameplayTag& AbilityTag, bool bActivate)
{
	if (!PassiveSpellTag.MatchesTagExact(AbilityTag)) return;

	AActor* Owner = GetOwner();
	bool bOwnerValid = IsValid(Owner);
	bool bOwnerAlive = bOwnerValid && Owner->Implements<UCombatInterface>() && !ICombatInterface::Execute_IsDead(Owner);

	if (bActivate && bOwnerAlive)
	{
		if (!IsActive()) Activate();
	}
	else
	{
		Deactivate();
	}

}

void UPassiveNiagaraComponent::OnOwnerDeath(AActor* DeadActor)
{
	Deactivate();
}

void UPassiveNiagaraComponent::ActivateIfEquiped(UAuraAbilitySystemComponent* AuraASC)
{
	if (AuraASC->bStartupAbilitiesGiven)
	{
		if (AuraASC->GetStatusFromAbilityTag(PassiveSpellTag) == FAuraGameplayTags::Get().Abilities_Status_Equipped)
		{
			Activate();
		}
	}
}
