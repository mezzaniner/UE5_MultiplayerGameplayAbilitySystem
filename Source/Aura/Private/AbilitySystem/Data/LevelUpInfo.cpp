// Copyright Monkeyman Studios


#include "AbilitySystem/Data/LevelUpInfo.h"

int32 ULevelUpInfo::FindLevelForXP(int32 XP) const
{
	int32 Level = 1;
	bool bSearching = true;
	while (bSearching)
	{
		// LeveUpInformation at 1 = Level 1 information
		// LeveUpInformation at 2 = Level 2 information...
		if (LevelUpInformation.Num() - 1 <= Level) return Level;

		if (XP >= LevelUpInformation[Level].LevelUpRequiredXP)
		{
			++Level;
		}
		else
		{
			bSearching = false;
		}
	}

	return Level;
}
