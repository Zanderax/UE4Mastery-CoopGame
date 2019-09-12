// Fill out your copyright notice in the Description page of Project Settings.


#include "SHitscanWeapon.h"

#include "CoopGame.h"

#include "DrawDebugHelpers.h"
#include "Engine\EngineTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials\PhysicalMaterial.h"
#include "TimerManager.h"

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(
	TEXT("COOP.DebugWeapons"),
	DebugWeaponDrawing,
	TEXT("Draw Debug Lines for Weapons"),
	ECVF_Cheat);

ASHitscanWeapon::ASHitscanWeapon()
{
	TracerTargetName = "Target";

	BaseDamage = 20.0f;
	HeadshotMultiplier = 4.0f;
	RateOfFire = 600;
}

void ASHitscanWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASHitscanWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}

void ASHitscanWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

void ASHitscanWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60 / RateOfFire;

	LastFireTime = -TimeBetweenShots;
}

void ASHitscanWeapon::Fire()
{
	Super::Fire();

	//Trace world from pawn eyes to crosshair location (centre screen)
	AActor* MyOwner = GetOwner();

	if (!MyOwner)
	{
		return;
	}

	FVector EyeLocation;
	FRotator EyeRotation;

	MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	FVector ShotDirection = EyeRotation.Vector();
	FVector TraceEnd = EyeLocation + (ShotDirection * 10000);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(MyOwner);
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = true;
	QueryParams.bReturnPhysicalMaterial = true;

	// Particle "Target" Parameter
	FVector TracerEndPoint = TraceEnd;

	EPhysicalSurface SurfaceType = SurfaceType_Default;

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams))
	{
		//Blocking hit, process damage

		AActor* HitActor = Hit.GetActor();


		SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

		float ActualDamage = BaseDamage;

		if (SurfaceType == SURFACE_FLESH_VULNERABLE)
		{
			ActualDamage *= HeadshotMultiplier;
		}

		if (Role == ROLE_Authority)
		{
			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), this, DamageType);
		}

		PlayImpactFx(SurfaceType, Hit.ImpactPoint);

		TracerEndPoint = Hit.ImpactPoint;
	}
	if (DebugWeaponDrawing > 0)
	{
		DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::White, false, 1.0f, 0, 1.0f);
	}
	
	PlayTracerFx(TracerEndPoint);

	LastFireTime = GetWorld()->TimeSeconds;

	if (Role == ROLE_Authority)
	{
		HitScanTrace.TraceTo = TracerEndPoint;
		HitScanTrace.SurfaceType = SurfaceType;
	}
}

void ASHitscanWeapon::PlayTracerFx(FVector TracerEndPoint)
{
	if (TracerFx)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		UParticleSystemComponent * TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerFx, MuzzleLocation);
		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TracerTargetName, TracerEndPoint);
		}
	}
}

void ASHitscanWeapon::PlayImpactFx(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	UParticleSystem* SelectedEffect = nullptr;

	switch (SurfaceType)
	{
	case SURFACE_FLESH_DEFAULT:
	case SURFACE_FLESH_VULNERABLE:
		SelectedEffect = FleshImpactFx;
		break;
	default:
		SelectedEffect = DefaultImpactFx;
		break;
	}

	if (SelectedEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		ShotDirection.Normalize();

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
	}
}

void ASHitscanWeapon::OnRep_HitScanTrace()
{
	// Play Cosmetic FX
	PlayTracerFx(HitScanTrace.TraceTo);

	PlayImpactFx(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}

void ASHitscanWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASHitscanWeapon, HitScanTrace, COND_SkipOwner);
}
