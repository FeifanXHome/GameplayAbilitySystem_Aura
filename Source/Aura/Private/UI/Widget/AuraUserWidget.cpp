// Copyright XXX


#include "UI/Widget/AuraUserWidget.h"
#include "UI/WidgetController/AuraWidgetController.h"
#include "Kismet/KismetSystemLibrary.h"

void UAuraUserWidget::SetWidgetController(UObject* InWidgetController)
{
	WidgetController = InWidgetController;
	WidgetControllerSet();
}

void UAuraUserWidget::NativeDestruct()
{
	Super::NativeDestruct();

	UAuraWidgetController* AuraWidgetController = Cast<UAuraWidgetController>(WidgetController);
	//checkf(AuraWidgetController, TEXT("[%s]:: WidgetController is nullptr !"), *GetNameSafe(this));
	if (AuraWidgetController == nullptr)
	{
		//FString msg = FString::Printf(TEXT("[%s]:: WidgetController is nullptr !"), *GetNameSafe(this));
		//UKismetSystemLibrary::PrintString(this, msg, true, true, FLinearColor::Red, 10.f);
		return;
	}
	AuraWidgetController->OnWidgetDestruct(this);
}
