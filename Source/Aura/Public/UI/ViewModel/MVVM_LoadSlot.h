// Copyright XXX

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Game/LoadScreenSaveGame.h"
#include "MVVM_LoadSlot.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSetWidgetSwitcherIndex, int32, WidgetSwitcherIndex);

/**
 * 
 */
UCLASS()
class AURA_API UMVVM_LoadSlot : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintAssignable)
	FSetWidgetSwitcherIndex SetWidgetSwitcherIndex;

	void InitializeSlot();

	UPROPERTY()
	FString LoadSlotName = FString();

	UPROPERTY()
	int32 SlotIndex = 0;

	UPROPERTY()
	TEnumAsByte<ESaveSlotStatus> SlotStatus;

	/** Field Notifies */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	FString PlayerName = FString();

	void SetPlayerName(FString InPlayerName);
	FString GetPlayerName() const { return PlayerName; }

};
