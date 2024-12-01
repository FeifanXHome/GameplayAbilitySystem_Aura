// Copyright XXX


#include "AbilitySystem/Data/LevelUpInfo.h"

int32 ULevelUpInfo::FindLevelForXP(int32 XP) const
{
	int32 Leve = 1;

	bool bSearching = true;
	while (bSearching)
	{
		// LevelUpInformation[1] = Level 1 Information
		// LevelUpInformation[2] = Level 2 Information
		if (LevelUpInformation.Num() - 1 <= Leve) return Leve;

		if (XP >= LevelUpInformation[Leve].LevelUpRequirement)
		{
			++Leve;
		}
		else
		{
			bSearching = false;
		}
	}

	return Leve;
}
