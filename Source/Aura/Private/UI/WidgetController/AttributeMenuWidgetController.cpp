// Copyright XXX


#include "UI/WidgetController/AttributeMenuWidgetController.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/Data/AttributeInfo.h"
#include "AuraGameplayTags.h"

void UAttributeMenuWidgetController::BroadcastInitialValues()
{
	const UAuraAttributeSet* AS = CastChecked<UAuraAttributeSet>(AttributeSet);

	check(AttributeInfo);

// 	FGameplayTag Tag = FAuraGameplayTags::Get().Attributes_Primary_Strength;
// 
// 	FAuraAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(Tag);
// 	Info.AttributeValue = AS->GetStrength();
// 
// 	AttributeInfoDelegate.Broadcast(Info);

	for (auto& Pair : AS->TagsToAttributes)
	{
		FAuraAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(Pair.Key);
		Info.AttributeValue = Pair.Value().GetNumericValue(AS);
		AttributeInfoDelegate.Broadcast(Info);
	}

}

void UAttributeMenuWidgetController::BindCallbacksToDependencies()
{
}
