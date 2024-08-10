// Copyright XXX

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "AuraInputConfig.h"
#include "AuraInputComponent.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()
	
public:
	template<typename UserClass, class PressedFunctype, typename ReleasedFunctype, typename HeldFuncType>
	void BindAbilityActions(const UAuraInputConfig* InputConfig, UserClass* Object, PressedFunctype PressedFunc, ReleasedFunctype ReleasedFunc, HeldFuncType HeldFunc);
};

template<typename UserClass, class PressedFunctype, typename ReleasedFunctype, typename HeldFuncType>
inline void UAuraInputComponent::BindAbilityActions(const UAuraInputConfig* InputConfig, UserClass* Object, 
	PressedFunctype PressedFunc, ReleasedFunctype ReleasedFunc, HeldFuncType HeldFunc)
{
	check(InputConfig);

	for (const FAuraInputAction& Action : InputConfig->AbilityInputAction)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if (PressedFunc)  BindAction(Action.InputAction, ETriggerEvent::Started,   Object, PressedFunc,  Action.InputTag);
			if (ReleasedFunc) BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag);
			if (HeldFunc)	  BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, HeldFunc,	 Action.InputTag);
		}
	}
}
