// Copyright XXX


#include "Actor/PointCollection.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
APointCollection::APointCollection()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Pt_0 = CreateDefaultSubobject<USceneComponent>(TEXT("Pt_0"));
	SetRootComponent(Pt_0);
	ImmutablePts.Add(Pt_0);

	Pt_1 = CreateDefaultSubobject<USceneComponent>(TEXT("Pt_1"));
	Pt_1->SetupAttachment(GetRootComponent());
	ImmutablePts.Add(Pt_1);

	Pt_2 = CreateDefaultSubobject<USceneComponent>(TEXT("Pt_2"));
	Pt_2->SetupAttachment(GetRootComponent());
	ImmutablePts.Add(Pt_2);

	Pt_3 = CreateDefaultSubobject<USceneComponent>(TEXT("Pt_3"));
	Pt_3->SetupAttachment(GetRootComponent());
	ImmutablePts.Add(Pt_3);

	Pt_4 = CreateDefaultSubobject<USceneComponent>(TEXT("Pt_4"));
	Pt_4->SetupAttachment(GetRootComponent());
	ImmutablePts.Add(Pt_4);

	Pt_5 = CreateDefaultSubobject<USceneComponent>(TEXT("Pt_5"));
	Pt_5->SetupAttachment(GetRootComponent());
	ImmutablePts.Add(Pt_5);

	Pt_6 = CreateDefaultSubobject<USceneComponent>(TEXT("Pt_6"));
	Pt_6->SetupAttachment(GetRootComponent());
	ImmutablePts.Add(Pt_6);

	Pt_7 = CreateDefaultSubobject<USceneComponent>(TEXT("Pt_7"));
	Pt_7->SetupAttachment(GetRootComponent());
	ImmutablePts.Add(Pt_7);

	Pt_8 = CreateDefaultSubobject<USceneComponent>(TEXT("Pt_8"));
	Pt_8->SetupAttachment(GetRootComponent());
	ImmutablePts.Add(Pt_8);

	Pt_9 = CreateDefaultSubobject<USceneComponent>(TEXT("Pt_9"));
	Pt_9->SetupAttachment(GetRootComponent());
	ImmutablePts.Add(Pt_9);

	Pt_10 = CreateDefaultSubobject<USceneComponent>(TEXT("Pt_10"));
	Pt_10->SetupAttachment(GetRootComponent());
	ImmutablePts.Add(Pt_10);

}

/** Util for drawing result of single line trace  */
void DrawDebugLineTraceSingle(const UWorld* World, const FVector& Start, const FVector& End, EDrawDebugTrace::Type DrawDebugType, bool bHit, const FHitResult& OutHit, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	static const float KISMET_TRACE_DEBUG_IMPACTPOINT_SIZE = 16.f;
	if (DrawDebugType != EDrawDebugTrace::None)
	{
		bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
		float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

		// @fixme, draw line with thickness = 2.f?
		if (bHit && OutHit.bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			::DrawDebugLine(World, Start, OutHit.ImpactPoint, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugLine(World, OutHit.ImpactPoint, End, TraceHitColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugPoint(World, OutHit.ImpactPoint, KISMET_TRACE_DEBUG_IMPACTPOINT_SIZE, TraceColor.ToFColor(true), bPersistent, LifeTime);
		}
		else
		{
			// no hit means all red
			::DrawDebugLine(World, Start, End, TraceColor.ToFColor(true), bPersistent, LifeTime);
		}
	}
}

bool LineTraceSingleByProfile(const UObject* WorldContextObject, 
	struct FHitResult& OutHit, const FVector& Start, const FVector& End, FName ProfileName, const struct FCollisionQueryParams& Params,
	EDrawDebugTrace::Type DrawDebugType = EDrawDebugTrace::None, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	bool const bHit = World ? World->LineTraceSingleByProfile(OutHit, Start, End, ProfileName, Params) : false;
	
	DrawDebugLineTraceSingle(World, Start, End, DrawDebugType, bHit, OutHit, TraceColor, TraceHitColor, DrawTime);

	return bHit;
}

TArray<USceneComponent*> APointCollection::GetGroundPoints1(const FVector& GroundLocation, int32 NumPoints, float YawOverride, bool IsShowDebugShapes, float DebugDrawTime)
{
	return GetGroundPoints(GroundLocation, NumPoints, YawOverride, IsShowDebugShapes, DebugDrawTime);
}

TArray<USceneComponent*> APointCollection::GetGroundPoints(const FVector& GroundLocation, int32 NumPoints, float YawOverride, bool IsShowDebugShapes, float DebugDrawTime)
{
	checkf( ImmutablePts.Num()>=NumPoints, TEXT("Attempted to accesss ImmutablePts out of bounds."));
	TArray<USceneComponent*> ArrayCopy;
	
	const FVector Location_Pt_0 = Pt_0->GetComponentLocation();
	const FVector Location_Actor = GetActorLocation();

	for (USceneComponent* Pt : ImmutablePts)
	{
		if (ArrayCopy.Num()>=NumPoints) break;

		FVector Location_Pt = Pt->GetComponentLocation();
		
		//
		if (Pt != Pt_0)
		{
			FVector ToPoint = Location_Pt - Location_Pt_0;
			ToPoint = ToPoint.RotateAngleAxis(YawOverride, FVector::UpVector);
			ToPoint = Location_Pt_0 + ToPoint;
			Pt->SetWorldLocation(ToPoint);

			Location_Pt = Pt->GetComponentLocation();
		}

		//
		TArray<AActor*> IgnoreActors;
		UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(this, IgnoreActors, TArray<AActor*>(), 1500.f, Location_Actor);

		//
		const FVector RaisedLocation  = Location_Pt + FVector(0, 0, 500.f);
		const FVector LoweredLocation = Location_Pt + FVector(0, 0, -500.f);

		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActors(IgnoreActors);
		LineTraceSingleByProfile(this, 
			HitResult, RaisedLocation, LoweredLocation, FName(TEXT("BlockAll")), QueryParams,
			IsShowDebugShapes ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None, 
			FLinearColor::Red, FLinearColor::Green, DebugDrawTime);

		//
		FVector AdjustedLocation = Location_Pt;
		//if (HitResult.bBlockingHit)
		{
			AdjustedLocation = FVector(Location_Pt.X, Location_Pt.Y, HitResult.ImpactPoint.Z);
			Pt->SetWorldLocation(AdjustedLocation);
			Pt->SetWorldRotation(UKismetMathLibrary::MakeRotFromZ(HitResult.ImpactNormal));
		}

		ArrayCopy.Add(Pt);
	}

	if (IsShowDebugShapes)
	{
		for (USceneComponent* Pt : ArrayCopy)
		{
			DrawDebugSphere(GetWorld(), Pt->GetComponentLocation(), 16.f, 8, FColor::Cyan, false, DebugDrawTime);
		}
	}
	
	return ArrayCopy;
}

TArray<USceneComponent*> APointCollection::GetGroundPoints2(const FVector& GroundLocation, int32 NumPoints, float YawOverride)
{
	checkf(ImmutablePts.Num() >= NumPoints, TEXT("Attempted to access ImmutablePts out of bounds."));

	TArray<USceneComponent*> ArrayCopy;

	for (USceneComponent* Pt : ImmutablePts)
	{
		if (ArrayCopy.Num() >= NumPoints) return ArrayCopy;

		if (Pt != Pt_0)
		{
			FVector ToPoint = Pt->GetComponentLocation() - Pt_0->GetComponentLocation();
			ToPoint = ToPoint.RotateAngleAxis(YawOverride, FVector::UpVector);
			Pt->SetWorldLocation(Pt_0->GetComponentLocation() + ToPoint);
		}

		const FVector RaisedLocation = FVector(Pt->GetComponentLocation().X, Pt->GetComponentLocation().Y, Pt->GetComponentLocation().Z + 500.f);
		const FVector LoweredLocation = FVector(Pt->GetComponentLocation().X, Pt->GetComponentLocation().Y, Pt->GetComponentLocation().Z - 500.f);

		FHitResult HitResult;
		TArray<AActor*> IgnoreActors;
		UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(this, IgnoreActors, TArray<AActor*>(), 1500.f, GetActorLocation());

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActors(IgnoreActors);
		GetWorld()->LineTraceSingleByProfile(HitResult, RaisedLocation, LoweredLocation, FName("BlockAll"), QueryParams);

		const FVector AdjustedLocation = FVector(Pt->GetComponentLocation().X, Pt->GetComponentLocation().Y, HitResult.ImpactPoint.Z);
		Pt->SetWorldLocation(AdjustedLocation);
		Pt->SetWorldRotation(UKismetMathLibrary::MakeRotFromZ(HitResult.ImpactNormal));

		ArrayCopy.Add(Pt);
	}
	return ArrayCopy;
}

// Called when the game starts or when spawned
void APointCollection::BeginPlay()
{
	Super::BeginPlay();
	
}
