// Copyright XXX


#include "Interaction/CombatInterface.h"

// Add default functionality here for any ICombatInterface functions that are not pure virtual.

int32 ICombatInterface::Execute_GetPlayerLevel_Check(UObject* O, int32 defaultValue)
{
	if (O && O->Implements<UCombatInterface>())
	{
		return ICombatInterface::Execute_GetPlayerLevel(O);
	}
	return defaultValue;
}
