// Copyright Monkeyman Studios


#include "AbilitySystem/Data/AttributeInfo.h"
#include "Aura/AuraLogChannels.h"


FAuraAttributeInfo UAttributeInfo::FindAttributeInfoByTag(const FGameplayTag& AttributeTag, bool bLogNotFound) const
{
	for (const FAuraAttributeInfo& Info : AttributeInformation)
	{
		if (Info.AttributeTag.MatchesTagExact(AttributeTag))
		{
			return Info;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogAura, Error, TEXT("AttributeInfo not found for tag [%s] on AttributeInfo [%s]"),
		  *AttributeTag.ToString(),
		  *GetNameSafe(this));
	}

	return FAuraAttributeInfo();
}
