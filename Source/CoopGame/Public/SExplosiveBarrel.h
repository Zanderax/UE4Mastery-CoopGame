// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SExplosiveBarrel.generated.h"

// Project Forward Declares
class USHealthComponent;

// Engine Forward Declares
class UMaterial;
class USphereComponent;

UCLASS()
class COOPGAME_API ASExplosiveBarrel : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASExplosiveBarrel();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly)
	UStaticMeshComponent* BarrelMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USHealthComponent* HealthComp;

	UPROPERTY(ReplicatedUsing = OnRep_bHasExploded)
	bool bHasExploded;

	UFUNCTION()
	void OnRep_bHasExploded();

	UPROPERTY(EditDefaultsOnly)
	UParticleSystem* ExplodeFx;

	UPROPERTY(EditDefaultsOnly)
	UMaterial* DefaultMaterial;

	UPROPERTY(EditDefaultsOnly)
	UMaterial* ExplodedMaterial;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* ExplosionRadius;

	UPROPERTY(EditAnywhere)
	float SelfPropulsionForce;

	UPROPERTY(EditAnywhere)
	float OtherPropulsionForce;

	UFUNCTION()
	void OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	void Explode();
};
