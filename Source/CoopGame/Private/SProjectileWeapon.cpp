// Fill out your copyright notice in the Description page of Project Settings.


#include "SProjectileWeapon.h"

#include "Kismet\GameplayStatics.h"

void ASProjectileWeapon::Fire()
{
	if (Role < ROLE_Authority)
	{
		ServerFire();
		return;
	}

	Super::Fire();

	const FVector SpawnLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
	APlayerCameraManager * CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const FRotator SpawnRotation = CameraManager->GetCameraRotation();
	AActor* ProjectileActor = GetWorld()->SpawnActor(ProjectileType, &SpawnLocation, &SpawnRotation);
}
