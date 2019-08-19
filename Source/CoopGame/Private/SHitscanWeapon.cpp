// Fill out your copyright notice in the Description page of Project Settings.


#include "SHitscanWeapon.h"

#include "CoopGame.h"

#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

#include "PhysicalMaterials\PhysicalMaterial.h"
#include "Engine\EngineTypes.h"

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

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams))
	{
		//Blocking hit, process damage

		AActor* HitActor = Hit.GetActor();


		EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

		float ActualDamage = BaseDamage;

		if (SurfaceType == SURFACE_FLESH_VULNERABLE)
		{
			ActualDamage *= HeadshotMultiplier;
		}

		UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), this, DamageType);

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
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
		}

		TracerEndPoint = Hit.ImpactPoint;
	}
	if (DebugWeaponDrawing > 0)
	{
		DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::White, false, 1.0f, 0, 1.0f);
	}
	
	PlayTracerFx(TracerEndPoint);

	LastFireTime = GetWorld()->TimeSeconds;
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
