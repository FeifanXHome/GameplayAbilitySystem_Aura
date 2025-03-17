// Copyright XXX

#pragma once

#include "CoreMinimal.h"
#include "NiagaraComponent.h"
#include "GameplayTagContainer.h"
#include "PassiveNiagaraComponent.generated.h"

class UAuraAbilitySystemComponent;

/**
 * 
 */
UCLASS()
class AURA_API UPassiveNiagaraComponent : public UNiagaraComponent
{
	GENERATED_BODY()

public:
	UPassiveNiagaraComponent();

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag PassiveSpellTag;

protected:
	virtual void BeginPlay() override;

	void OnPassiveActivate(const FGameplayTag& AbilityTag, bool bActivate);

	UFUNCTION()
	void OnOwnerDeath(AActor* DeadActor);

	void ActivateIfEquiped(UAuraAbilitySystemComponent* AuraASC);

};
