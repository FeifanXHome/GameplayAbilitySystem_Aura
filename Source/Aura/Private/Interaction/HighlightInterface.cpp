// Copyright XXX


#include "Interaction/HighlightInterface.h"

// Add default functionality here for any IHighlightInterface functions that are not pure virtual.

void IHighlightInterface::Execute_HighlightActor_Check(UObject* O)
{
	if (O && O->Implements<UHighlightInterface>())
	{
		IHighlightInterface::Execute_HighlightActor(O);
	}
}

void IHighlightInterface::Execute_UnHighlightActor_Check(UObject* O)
{
	if (O && O->Implements<UHighlightInterface>())
	{
		IHighlightInterface::Execute_UnHighlightActor(O);
	}
}

void IHighlightInterface::Execute_SetMoveToLocation_Check(UObject* O, FVector& OutDestination)
{
	if (O && O->Implements<UHighlightInterface>())
	{
		IHighlightInterface::Execute_SetMoveToLocation(O, OutDestination);
	}
}