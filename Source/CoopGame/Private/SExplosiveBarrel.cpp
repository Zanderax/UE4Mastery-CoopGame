// Fill out your copyright notice in the Description page of Project Settings.


#include "SExplosiveBarrel.h"

#include "SHealthComponent.h"

#include "Kismet/GameplayStatics.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

// Sets default values
ASExplosiveBarrel::ASExplosiveBarrel()
{
	SelfPropulsionForce = 200000.0f;
	OtherPropulsionForce = 200000.0f;

	BarrelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelMesh"));
	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	ExplosionRadius = CreateDefaultSubobject<USphereComponent>(TEXT("ExplosionRadius"));
}

// Called when the game starts or when spawned
void ASExplosiveBarrel::BeginPlay()
{
	Super::BeginPlay();

	if (DefaultMaterial)
	{
		BarrelMesh->SetMaterial(0, DefaultMaterial);
	}
	BarrelMesh->SetSimulatePhysics(true);
	HealthComp->OnHealthChanged.AddDynamic(this, &ASExplosiveBarrel::OnHealthChanged);
}

void ASExplosiveBarrel::OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.0f && !bHasExploded)
	{
		bHasExploded = true;
		Explode();
	}
}

void ASExplosiveBarrel::Explode()
{
	if (ExplodeFx)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplodeFx, GetActorLocation());
	}

	if (ExplodedMaterial)
	{
		BarrelMesh->SetMaterial(0, ExplodedMaterial);
	}

	BarrelMesh->AddForce(BarrelMesh->GetUpVector() * SelfPropulsionForce);

	TArray<UPrimitiveComponent*> CompsInExplosionRange;
	ExplosionRadius->GetOverlappingComponents(CompsInExplosionRange);
	for (UPrimitiveComponent* CompInExplosionRange : CompsInExplosionRange)
	{
		CompInExplosionRange->AddRadialForce(GetActorLocation(), ExplosionRadius->GetUnscaledSphereRadius(), OtherPropulsionForce, ERadialImpulseFalloff::RIF_Constant, true);
	}

}

