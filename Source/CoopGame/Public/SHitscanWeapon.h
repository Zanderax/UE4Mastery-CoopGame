// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SWeapon.h"
#include "SHitscanWeapon.generated.h"

// Contains information of a single hitscan weapon line
USTRUCT()
struct FHitScanTrace
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TEnumAsByte<EPhysicalSurface> SurfaceType;

	UPROPERTY()
	FVector_NetQuantize TraceTo;
};

/**
 * 
 */
UCLASS()
class COOPGAME_API ASHitscanWeapon : public ASWeapon
{
	GENERATED_BODY()
	
public:
	ASHitscanWeapon();

	void StartFire() override;

	void StopFire() override;

protected:
	virtual void BeginPlay() override;

	virtual void Fire() override;

	FTimerHandle TimerHandle_TimeBetweenShots;

	void PlayTracerFx(FVector TracerEndPoint);

	void PlayImpactFx(EPhysicalSurface SurfaceType, FVector ImpactPoint);

	UPROPERTY(EditDefaultsOnly)
	UParticleSystem* TracerFx;


	UPROPERTY(EditDefaultsOnly)
	UParticleSystem* DefaultImpactFx;

	UPROPERTY(EditDefaultsOnly)
	UParticleSystem* FleshImpactFx;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName TracerTargetName;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float BaseDamage;

	/* Bullet spread in Degrees*/
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin=0.0f))
	float BulletSpread; 

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float HeadshotMultiplier;

	float LastFireTime;

	/* RPM Bullets Per Minute */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float RateOfFire;

	// Derived from RateOfFire
	float TimeBetweenShots;

	UPROPERTY(ReplicatedUsing=OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

	UFUNCTION()
	void OnRep_HitScanTrace();
};
