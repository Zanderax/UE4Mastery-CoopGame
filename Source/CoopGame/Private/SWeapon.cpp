// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon.h"

#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"


// Sets default values
ASWeapon::ASWeapon()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";

	SetReplicates(true);

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
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
	if (Role < ROLE_Authority)
	{
		ServerFire();
	}

	PlayFireFx();
}

void ASWeapon::ServerFire_Implementation()
{
	Fire();
}

bool ASWeapon::ServerFire_Validate()
{
	return true;
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