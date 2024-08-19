// Copyright XXX


#include "AbilitySystem/AbilityTasks/TargetDataUnderMouse.h"
#include "Kismet/GameplayStatics.h"

UTargetDataUnderMouse* UTargetDataUnderMouse::CreateTargetDataUnderMouse(UGameplayAbility* OwningAbility)
{
	UTargetDataUnderMouse* MyObj = NewAbilityTask<UTargetDataUnderMouse>(OwningAbility);
	return MyObj;
}

void UTargetDataUnderMouse::Activate()
{

	APlayerController* PC = Ability->GetCurrentActorInfo()->PlayerController.Get();
	
	FHitResult CursorHit;
	PC->GetHitResultUnderCursor(ECC_Visibility, false, CursorHit);
	//if (!CursorHit.bBlockingHit) return;

	ValidData.Broadcast(CursorHit.Location);
}
