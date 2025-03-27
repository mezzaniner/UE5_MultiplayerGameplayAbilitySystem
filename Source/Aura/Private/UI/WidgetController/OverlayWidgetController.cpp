// Copyright Monkeyman Studios


#include "UI/WidgetController/OverlayWidgetController.h"

#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/Data/AbilityInfo.h"

void UOverlayWidgetController::BroadcastInitialValues()
{
	const UAuraAttributeSet* AuraAttributeSet = CastChecked<UAuraAttributeSet>(AttributeSet);

	OnHealthChanged.Broadcast(AuraAttributeSet->GetHealth());
	OnMaxHealthChanged.Broadcast(AuraAttributeSet->GetMaxHealth());
	OnManaChanged.Broadcast(AuraAttributeSet->GetMana());
	OnMaxManaChanged.Broadcast(AuraAttributeSet->GetMaxMana());
}

void UOverlayWidgetController::BindCallbacksToDependencies()
{
	const UAuraAttributeSet* AuraAttributeSet = CastChecked<UAuraAttributeSet>(AttributeSet);

	// Bind OnHealthChanged
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AuraAttributeSet->GetHealthAttribute())
		.AddLambda([this](const FOnAttributeChangeData& Data)
			{
				OnHealthChanged.Broadcast(Data.NewValue);
			}
		);

	// Bind OnMaxHealthChanged
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AuraAttributeSet->GetMaxHealthAttribute())
		.AddLambda([this](const FOnAttributeChangeData& Data)
			{
				OnMaxHealthChanged.Broadcast(Data.NewValue);
			}
		);

	// Bind OnManaChanged
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AuraAttributeSet->GetManaAttribute())
		.AddLambda([this](const FOnAttributeChangeData& Data)
			{
				OnManaChanged.Broadcast(Data.NewValue);
			}
		);

	// Bind OnMaxManaChanged
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AuraAttributeSet->GetMaxManaAttribute())
		.AddLambda([this](const FOnAttributeChangeData& Data)
			{
				OnMaxManaChanged.Broadcast(Data.NewValue);
			}
		);

	if (UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(AbilitySystemComponent))
	{
		// Handles cases where either the AbilitySystemComponent has Given the abilities yet or it hasn't
		if (AuraASC->bStartupAbilitiesGiven)
		{
			OnInitializeStartupAbilities(AuraASC);
		}
		else
		{
			AuraASC->AbilitiesGivenDelegate.AddUObject(this, &UOverlayWidgetController::OnInitializeStartupAbilities);
		}
	
		// Bind to EffectAssetTagsDelegate
		AuraASC->EffectAssetTagsDelegate
			.AddLambda([this](const FGameplayTagContainer& AssetTags)
				{
					for (const FGameplayTag& Tag : AssetTags)
					{
						// "Message.HealthPotion".MatchesTag("Message") will return True
						// "Message".MatchesTag("Message.HealthPotion") will return False
						FGameplayTag MessageTag = FGameplayTag::RequestGameplayTag(FName("Message"));
						if (!Tag.MatchesTag(MessageTag)) return;

						const FUIWidgetRow* Row = GetDataTableRowByTag<FUIWidgetRow>(MessageWidgetDataTable, Tag);
						MessageWidgetRowDelegate.Broadcast(*Row);
					}
				}	
			);
	}
	

}

void UOverlayWidgetController::OnInitializeStartupAbilities(UAuraAbilitySystemComponent* AuraAbilitySystemComponent)
{
	//TODO: Get information about all given abilities, look up their Ability Info, and broadcast it to widgets
	if (!AuraAbilitySystemComponent->bStartupAbilitiesGiven) return;

	FForEachAbility BroadcastDelegate;
	BroadcastDelegate.BindLambda([this, AuraAbilitySystemComponent](const FGameplayAbilitySpec& AbilitySpec)
	{
		//TODO: need a way to figure out the ability tag for a given ability spec
		FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(AuraAbilitySystemComponent->GetAbilityTagFromSpec(AbilitySpec));
		Info.InputTag = AuraAbilitySystemComponent->GetInputTagFromSpec(AbilitySpec);
		AbilityInfoDelegate.Broadcast(Info);
	});

	AuraAbilitySystemComponent->ForEachAbility(BroadcastDelegate);
}
