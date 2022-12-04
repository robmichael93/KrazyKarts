// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMovementComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GoKart.h"
#include "GoKartMovementReplicator.h"

// Sets default values for this component's properties
UGoKartMovementComponent::UGoKartMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

// Called when the game starts
void UGoKartMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	MovementReplicator = GetOwner()->FindComponentByClass<UGoKartMovementReplicator>();
	ClientSimulatedTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
}

// Called every frame
void UGoKartMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ENetRole Role = GetOwnerRole();
    bool bIsLocallyControlled = Cast<APawn>(GetOwner())->IsLocallyControlled();

    CurrentMove = CreateMove(DeltaTime);

    if (Role == ROLE_AutonomousProxy)
    {
        SimulateMove(CurrentMove);
        Server_SimulateMove(CurrentMove);
    }
    else if (Role == ROLE_Authority && bIsLocallyControlled)
    {
        SimulateMove(CurrentMove);
    }
    else if (Role == ROLE_SimulatedProxy && MovementReplicator)
    {
        SimulateMove(MovementReplicator->GetServerState().LastMove);
    }
}

FGoKartMove UGoKartMovementComponent::CreateMove(float DeltaTime)
{
    FGoKartMove Move;
    Move.SteeringThrow = SteeringThrow;
    Move.Throttle = Throttle;
    Move.DeltaTime = DeltaTime;
    Move.Time = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
    
    return Move;
}

void UGoKartMovementComponent::SimulateMove(const FGoKartMove& Move)
{
    FVector Force = GetOwner()->GetActorForwardVector() * DrivingForce * Move.Throttle;

    Force += CalculateAirResistance();
    Force += CalculateRollingResistance();
    
    Acceleration = Force / Mass;
    Velocity += Acceleration * Move.DeltaTime;

    ApplyRotation(Move.DeltaTime, Move.SteeringThrow);
    UpdateLocationFromVelocity(Move.DeltaTime);
}

void UGoKartMovementComponent::Server_SimulateMove_Implementation(const FGoKartMove& Move)
{
    ClientSimulatedTime += Move.DeltaTime;
    SimulateMove(Move);
}

bool UGoKartMovementComponent::Server_SimulateMove_Validate(const FGoKartMove& Move)
{
    if (!Move.IsValid())
    {
        return false;
    }
    else if (ClientSimulatedTime + Move.DeltaTime > GetWorld()->GetGameState()->GetServerWorldTimeSeconds())
    {
        UE_LOG(LogTemp, Error, TEXT("Client tried to move faster by cheating."));
        return false;
    }
    return true;
}

void UGoKartMovementComponent::ApplyRotation(float DeltaTime, float InSteeringThrow)
{
    float RotationAngle = FVector::DotProduct(GetOwner()->GetActorForwardVector(), Velocity) * DeltaTime * InSteeringThrow / TurningRadius;
    FQuat RotationDelta(GetOwner()->GetActorUpVector(), RotationAngle);

    Velocity = RotationDelta.RotateVector(Velocity);
    GetOwner()->AddActorWorldRotation(RotationDelta);
}

void UGoKartMovementComponent::UpdateLocationFromVelocity(float DeltaTime)
{
    FVector DeltaLocation = Velocity * DeltaTime * 100;
    FHitResult Collision;
    GetOwner()->AddActorWorldOffset(DeltaLocation, true, &Collision);
    if (Collision.bBlockingHit)
    {
        Velocity = FVector::ZeroVector;
    }
}

FVector UGoKartMovementComponent::CalculateAirResistance()
{
    return - Velocity.SizeSquared() * DragCoefficient * Velocity.GetSafeNormal();
}

FVector UGoKartMovementComponent::CalculateRollingResistance()
{
    return -Velocity.GetSafeNormal() * RollingResistanceCoefficient * Mass * -GetWorld()->GetGravityZ() / 100.f;
}