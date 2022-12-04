// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"
#include "GoKartMovementComponent.h"
#include "GoKartMovementReplicator.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>("Movement Component");
    MovementReplicator = CreateDefaultSubobject<UGoKartMovementReplicator>("Movement Replicator");
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();

    SetReplicateMovement(false);
    if (HasAuthority())
    {
        NetUpdateFrequency = 1.f;
    }
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::Move);
    PlayerInputComponent->BindAxis("Turn", this, &AGoKart::Turn);
}

void AGoKart::Move(float Value)
{
    if (MovementComponent)
    {
        MovementComponent->SetThrottle(Value);
    }
}

void AGoKart::Turn(float Value)
{
    if (MovementComponent)
    {
        MovementComponent->SetSteeringThrow(Value);
    }
}