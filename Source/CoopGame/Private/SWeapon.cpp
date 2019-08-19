// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon.h"

#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystem.h"


// Sets default values
ASWeapon::ASWeapon()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
}

void ASWeapon::StartFire()
{
	Fire();
}

void ASWeapon::StopFire()
{
}

void ASWeapon::Fire()
{
	PlayFireFx();
}

void ASWeapon::PlayFireFx()
{
	if (MuzzleFx)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleFx, MeshComp, MuzzleSocketName);
	}

	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(CameraShakeType);
		}
	}
}
