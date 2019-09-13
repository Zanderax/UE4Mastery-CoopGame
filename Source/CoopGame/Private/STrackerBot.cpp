// Fill out your copyright notice in the Description page of Project Settings.


#include "STrackerBot.h"

#include "SCharacter.h"
#include "SHealthComponent.h"

#include "DrawDebugHelpers.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Sound\SoundCue.h"


// Sets default values
ASTrackerBot::ASTrackerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetSimulatePhysics(true);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);

	PlayerDetectionRadius = 200;
	PlayerDetectionComp = CreateDefaultSubobject<USphereComponent>(TEXT("PlayerDetectionComp"));
	PlayerDetectionComp->SetSphereRadius(PlayerDetectionRadius);
	PlayerDetectionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PlayerDetectionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	PlayerDetectionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PlayerDetectionComp->OnComponentBeginOverlap.AddDynamic(this, &ASTrackerBot::OnPlayerDetectionCompOverlapBegin);
	PlayerDetectionComp->SetupAttachment(RootComponent);

	BotDetectionRadius = 400;
	BotDetectionComp = CreateDefaultSubobject<USphereComponent>(TEXT("BotDetectionComp"));
	BotDetectionComp->SetSphereRadius(BotDetectionRadius);
	BotDetectionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BotDetectionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	BotDetectionComp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	BotDetectionComp->OnComponentBeginOverlap.AddDynamic(this, &ASTrackerBot::OnBotDetectionCompOverlapBegin);
	BotDetectionComp->OnComponentEndOverlap.AddDynamic(this, &ASTrackerBot::OnBotDetectionCompOverlapEnd);
	BotDetectionComp->SetupAttachment(RootComponent);

	bUseVelocityChange = false;
	MovementForce = 1000;
	RequireDistanceToTarget = 100;

	ExplosionDamage = 40;
	ExplosionRadius = 200;

	SelfDamage = 40;
	SelfDamageInterval = 0.5f;

	SetReplicates(true);
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	if (Role == ROLE_Authority)
	{
		// Find inital move to
		NextPathPoint = GetNextPathPoint();

		InitDetectedBots();
	}
}

FVector ASTrackerBot::GetNextPathPoint()
{
	ACharacter* PlayerPawn = UGameplayStatics::GetPlayerCharacter(this, 0);
	
	UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), PlayerPawn);
	
	if (NavPath != nullptr && NavPath->PathPoints.Num() > 1)
	{
		return NavPath->PathPoints[1];
	}

	return GetActorLocation();
}

void ASTrackerBot::HandleTakeDamage(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{

	// Pulse the material on hit
	if (!MatInst)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst)
	{
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}

	// Explode on HP == 0!
	if (Health <= 0.f)
	{
		SelfDestruct();
	}
}

void ASTrackerBot::SelfDestruct()
{
	if (bExploded)
	{
		return;
	}

	bExploded = true;

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());

	MeshComp->SetVisibility(false);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (Role == ROLE_Authority)
	{

		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		auto TotalDamage = ExplosionDamage * ( BotsInDetectionRadius + 1 );

		UGameplayStatics::ApplyRadialDamage(this, TotalDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true);

		// Show explosion radius
		//DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Yellow, false, 2.f, 0, 1.f);


		SetLifeSpan(2.f);
	}
}

void ASTrackerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, SelfDamage, GetInstigatorController(), this, nullptr);
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Role == ROLE_Authority && !bExploded)
	{

		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

		//if (DistanceToTarget <= RequireDistanceToTarget || NextPathPoint.Equals(GetActorLocation(), 10) )
		//{
		//	
		//}
		NextPathPoint = GetNextPathPoint();
		//Keep moving to next target
		FVector ForceDirection = NextPathPoint - GetActorLocation();
		ForceDirection.Normalize();

		ForceDirection *= MovementForce;

		MeshComp->AddForce(ForceDirection, NAME_None, bUseVelocityChange);

			//DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32, FColor::Blue, false, 0.f, 0, 1.f);

		//DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Yellow, false, 4.f, 1.f);
	}
}

void ASTrackerBot::InitDetectedBots()
{
	if (Role == ROLE_Authority)
	{
		TArray<AActor*> OverlappingBots;
		BotDetectionComp->GetOverlappingActors(OverlappingBots, TSubclassOf<ASTrackerBot>());

		BotsInDetectionRadius = OverlappingBots.Num();
		UpdateMaterialWithDetectedBots();
	}
}

void ASTrackerBot::UpdateMaterialWithDetectedBots()
{
	if (!MatInst)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst)
	{
		MatInst->SetScalarParameterValue("BotsInDetectionRadius", BotsInDetectionRadius);
		UE_LOG(LogTemp, Log, TEXT("Update BotsInDetectionRadius = %d, On %s"), BotsInDetectionRadius, *UKismetSystemLibrary::GetDisplayName(this))
	}
}

void ASTrackerBot::OnRep_BotsInDetectionRadius()
{
	UpdateMaterialWithDetectedBots();
}

void ASTrackerBot::OnPlayerDetectionCompOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if(GetWorldTimerManager().IsTimerActive(TimerHandle_SelfDamage) || bExploded)
	{
		return;
	}
	ASCharacter* PlayerPawn = Cast<ASCharacter>(OtherActor);

	if (PlayerPawn)
	{
		if (Role == ROLE_Authority)
		{
			//Start self destruction sequence.
			GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 0.f);
		}

		UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
	}
}

void ASTrackerBot::OnBotDetectionCompOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (Role < ROLE_Authority)
	{
		return;
	}

	ASTrackerBot* TrackerBot = Cast<ASTrackerBot>(OtherActor);

	if (!TrackerBot)
	{
		return;
	}

	BotsInDetectionRadius++;
	UpdateMaterialWithDetectedBots();
}

void ASTrackerBot::OnBotDetectionCompOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Role < ROLE_Authority)
	{
		return;
	}

	ASTrackerBot* TrackerBot = Cast<ASTrackerBot>(OtherActor);

	if (!TrackerBot)
	{
		return;
	}

	BotsInDetectionRadius--;
	UpdateMaterialWithDetectedBots();
}

void ASTrackerBot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASTrackerBot, BotsInDetectionRadius);
}
