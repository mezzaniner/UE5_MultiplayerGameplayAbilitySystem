// Copyright Monkeyman Studios

#include "Character/AuraCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Aura/Aura.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"


AAuraCharacterBase::AAuraCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>("Weapon");
	Weapon->SetupAttachment(GetMesh(), FName("WeaponHandSocket"));
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AAuraCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

UAbilitySystemComponent* AAuraCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAnimMontage* AAuraCharacterBase::GetHitReactMontage_Implementation()
{
	return HitReactMontage;
}

// Called only on the server
void AAuraCharacterBase::Die()
{
	// Weapon->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));
	// Weapon->SetSimulatePhysics(true);
	// Weapon->SetEnableGravity(true);
	// Weapon->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	// GetMesh()->SetSimulatePhysics(true);
	// GetMesh()->SetEnableGravity(true);
	// GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	// GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	MulticastHandleDeath();
}

// Called on server and all clients
void AAuraCharacterBase::MulticastHandleDeath_Implementation()
{
	UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation(), GetActorRotation());

	Weapon->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));
	Weapon->SetSimulatePhysics(true);
	Weapon->SetEnableGravity(true);
	Weapon->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetEnableGravity(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Dissolve();
	bDead = true;
}

FVector AAuraCharacterBase::GetCombatSocketLocation_Implementation(const FGameplayTag& SocketTag)
{
	// TODO: Return correct socket based on MontageTag
	// TODO: Make a TMap mapping MontageTags to Sockets to make this data-driven

	const FAuraGameplayTags& AuraGameplayTags = FAuraGameplayTags::Get();
	
	if (SocketTag.MatchesTagExact(AuraGameplayTags.CombatSocket_Weapon) && IsValid(Weapon))
    {
        return Weapon->GetSocketLocation(WeaponTipSocketName);
    }
	
	if (SocketTag.MatchesTagExact(AuraGameplayTags.CombatSocket_LeftHand))
	{
		return GetMesh()->GetSocketLocation(LeftHandSocketName);
	}
	
	if (SocketTag.MatchesTagExact(AuraGameplayTags.CombatSocket_RightHand))
	{
		return GetMesh()->GetSocketLocation(RightHandSocketName);
	}
	
	if (SocketTag.MatchesTagExact(AuraGameplayTags.CombatSocket_Tail))
	{
		return GetMesh()->GetSocketLocation(TailSocketName);
	}

	return FVector();
}

bool AAuraCharacterBase::IsDead_Implementation() const
{
	return bDead;
}

AActor* AAuraCharacterBase::GetAvatar_Implementation()
{
	return this;
}

TArray<FTaggedMontage> AAuraCharacterBase::GetAttackMontages_Implementation()
{
	return AttackMontages;
}

UNiagaraSystem* AAuraCharacterBase::GetBloodEffect_Implementation()
{
	return BloodEffect;
}

FTaggedMontage AAuraCharacterBase::GetTaggedMontageByTag_Implementation(const FGameplayTag& MontageTag)
{
	for (FTaggedMontage TaggedMontage : AttackMontages)
	{
		if (TaggedMontage.MontageTag == MontageTag)
		{
			return TaggedMontage;
		}
	}

	return FTaggedMontage();
}

int32 AAuraCharacterBase::GetMinionCount_Implementation()
{
	return MinionCount;
}

void AAuraCharacterBase::IncrementMinionCount_Implementation(int32 Amount)
{
	MinionCount += Amount;
}

void AAuraCharacterBase::DecrementMinionCount_Implementation(int32 Amount)
{
	MinionCount -= Amount;
}

ECharacterClass AAuraCharacterBase::GetCharacterClass_Implementation()
{
	return CharacterClass;
}

void AAuraCharacterBase::InitAbilityActorInfo()
{
}

void AAuraCharacterBase::ApplyEffectToSelf(const TSubclassOf<UGameplayEffect>& GameplayEffectClass, const float Level) const
{
	check(IsValid(AbilitySystemComponent));
	checkf(GameplayEffectClass, TEXT("DefaultPrimaryAttributes is not set in Blueprint"));

	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	ContextHandle.AddSourceObject(this); // Add this actor as the source object
	const FGameplayEffectSpecHandle EffectSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(GameplayEffectClass,
	                                                                                            Level, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*EffectSpecHandle.Data.Get(), GetAbilitySystemComponent());
}

// These attributes are replicated and only need to be called on the server, but can optionally be called on clients
void AAuraCharacterBase::InitializeDefaultAttributes()
{
	ApplyEffectToSelf(DefaultPrimaryAttributes, 1.f);
	ApplyEffectToSelf(DefaultSecondaryAttributes, 1.f);
	ApplyEffectToSelf(DefaultVitalAttributes, 1.f);
}

void AAuraCharacterBase::AddCharacterAbilities()
{
	UAuraAbilitySystemComponent* AuraAbilitySystemComponent = CastChecked<UAuraAbilitySystemComponent>(AbilitySystemComponent);
	if (!HasAuthority()) return;

	AuraAbilitySystemComponent->AddCharacterAbilities(StartingAbilityClasses);
	AuraAbilitySystemComponent->AddCharacterPassiveAbilities(StartingPassiveAbilityClasses);
}

void AAuraCharacterBase::Dissolve()
{
	TArray<UMaterialInstanceDynamic*> DynamicMaterialInstances;
	
	if (IsValid(MeshDissolveMI))
	{
		UMaterialInstanceDynamic* MeshDynamicMI = UMaterialInstanceDynamic::Create(MeshDissolveMI, this);
		GetMesh()->SetMaterial(0, MeshDynamicMI);
		DynamicMaterialInstances.AddUnique(MeshDynamicMI);
	}

	if (IsValid(WeaponDissolveMI))
	{
		UMaterialInstanceDynamic* WeaponDynamicMI = UMaterialInstanceDynamic::Create(WeaponDissolveMI, this);
        Weapon->SetMaterial(0, WeaponDynamicMI);
		DynamicMaterialInstances.AddUnique(WeaponDynamicMI);
	}

	if (DynamicMaterialInstances.Num() > 0) StartDissolveTimeline(DynamicMaterialInstances);
}
