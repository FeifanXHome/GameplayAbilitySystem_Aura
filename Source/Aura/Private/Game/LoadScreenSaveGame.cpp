// Copyright XXX


#include "Game/LoadScreenSaveGame.h"

bool ULoadScreenSaveGame::GetSavedMapWithMapName(const FString& InMapName, FSavedMap& OutMap)
{
	if (InMapName.IsEmpty()) return false;

	for (const FSavedMap& Map : SavedMaps)
	{
		if (Map.MapAssetName == InMapName)
		{
            OutMap = Map;
            return true;
		}
	}

    return false;
}

FSavedMap ULoadScreenSaveGame::GetSavedMapWithMapName(const FString& InMapName)
{
	FSavedMap Map;
	const bool IsHasMap = GetSavedMapWithMapName(InMapName, Map);
    return Map;
}

bool ULoadScreenSaveGame::HasMap(const FString& InMapName)
{
	FSavedMap Map;
	const bool IsHasMap = GetSavedMapWithMapName(InMapName, Map);
	return IsHasMap;
}

void ULoadScreenSaveGame::AddOrUpdateSavedMap(const FSavedMap& InSavedMap)
{
	for (FSavedMap& Map : SavedMaps)
	{
		if (Map == InSavedMap)
		{
			Map = InSavedMap;
			return;
		}
	}

	SavedMaps.Add(InSavedMap);
}

