// Copyright XXX


#include "AbilitySystem/Abilities/AuraFireBolt.h"
#include "Interaction/CombatInterface.h"
#include "Kismet/KismetSystemLibrary.h"

FString UAuraFireBolt::GetDescription(int32 Level)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCooldown(Level);
	if (Level == 1)
	{
		return FString::Printf(TEXT(
			// Title
			"<Title>FIRE BOLT</>\n\n"

			// Level
			"<Small>Level: </><Level>%d</>\n"
			// ManaCost
			"<Small>ManaCost: </><ManaCost>%.1f</>\n"
			// Cooldown
			"<Small>Cooldown: </><Cooldown>%.1f</>\n\n"

			"<Default>Launches a bolt of fire, "
			"exploding on impact and dealing: </>"

			// Damage
			"<Damage>%d</><Default> fire damage with"
			" a chance to burn</>"),

			// Values
			Level,
			ManaCost,
			Cooldown,
			ScaledDamage);
	}
	else
	{
		return FString::Printf(TEXT(
			// Title
			"<Title>FIRE BOLT</>\n\n"

			// Level
			"<Small>Level: </><Level>%d</>\n"
			// ManaCost
			"<Small>ManaCost: </><ManaCost>%.1f</>\n"
			// Cooldown
			"<Small>Cooldown: </><Cooldown>%.1f</>\n\n"

			// Number of FireBolts
			"<Default>Launches %d bolts of fire, "
			"exploding on impact and dealing: </>"

			// Damage
			"<Damage>%d</><Default> fire damage with"
			" a chance to burn</>"),

			// Values
			Level,
			ManaCost,
			Cooldown,
			FMath::Min(Level, NumProjectiles),
			ScaledDamage);
	}
}

FString UAuraFireBolt::GetNextLevelDescription(int32 Level)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCooldown(Level);
	return FString::Printf(TEXT(
		// Title
		"<Title>NEXT LEVEL: </>\n\n"

		// Level
		"<Small>Level: </><Level>%d</>\n"
		// ManaCost
		"<Small>ManaCost: </><ManaCost>%.1f</>\n"
		// Cooldown
		"<Small>Cooldown: </><Cooldown>%.1f</>\n\n"

		// Number of FireBolts
		"<Default>Launches %d bolts of fire, "
		"exploding on impact and dealing: </>"

		// Damage
		"<Damage>%d</><Default> fire damage with"
		" a chance to burn</>"),

		// Values
		Level,
		ManaCost,
		Cooldown,
		FMath::Min(Level, NumProjectiles),
		ScaledDamage);
}

void UAuraFireBolt::SpawnProjectiles(const FVector ProjectileTargetLocation, const FGameplayTag SocketTag, bool bOverridePitch, float PitchOverride, AActor* HomingTaget, bool IsShowDebugShapes)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const bool bIsServer = AvatarActor->HasAuthority();
	if (!bIsServer) return;

	check(AvatarActor->Implements<UCombatInterface>());
	check(AvatarActor->GetClass()->ImplementsInterface(UCombatInterface::StaticClass()));
	const FVector SocketLocation = ICombatInterface::Execute_GetCombatSocketLocation(AvatarActor, SocketTag);

	FRotator Rotation = (ProjectileTargetLocation - SocketLocation).Rotation();
	Rotation.Pitch = bOverridePitch ? PitchOverride : 0.f;
	const FVector Forward = Rotation.Vector();

	if (IsShowDebugShapes) UKismetSystemLibrary::DrawDebugArrow(AvatarActor, SocketLocation, SocketLocation + Forward * 80.f, 18, FLinearColor::White, 13.f, 1);

	//NumProjectiles = FMath::Min(MaxNumProjectiles, GetAbilityLevel());
	//if (NumProjectiles == 1)
	//{
	//	SpawnProjectile(ProjectileTargetLocation, SocketTag, bOverridePitch, PitchOverride);
	//	return;
	//}

	check(NumProjectiles > 0);
	const float DeltaSpread = NumProjectiles == 0 ? 0 : ProjectileSpread / NumProjectiles;
	const FVector LeftOfSpread_ = Forward.RotateAngleAxis(-ProjectileSpread / 2.f, FVector::UpVector);

	for (int32 i = 0; i < NumProjectiles; i++)
	{
		const float Angle = DeltaSpread * (i + 0.5f);
		const FVector Direction = LeftOfSpread_.RotateAngleAxis(Angle, FVector::UpVector);

		if (IsShowDebugShapes)
		{
			UKismetSystemLibrary::DrawDebugArrow(AvatarActor, SocketLocation, SocketLocation + Direction * 100.f, 4, FLinearColor::MakeRandomColor(), 10.f);
		}
	}
}

