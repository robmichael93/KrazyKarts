// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMovementReplicator.h"
#include "DrawDebugHelpers.h"
#include "GoKart.h"
#include "Net/UnrealNetwork.h"

FString GetEnumText(ENetRole Role)
{
    switch (Role)
    {
        case ROLE_None:
            return "None";
        case ROLE_SimulatedProxy:
            return "SimulatedProxy";
        case ROLE_AutonomousProxy:
            return "AutonomousProxy";
        case ROLE_Authority:
            return "Authority";
        default:
            return "ERROR";
    }
}

// Sets default values for this component's properties
UGoKartMovementReplicator::UGoKartMovementReplicator()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}


// Called when the game starts
void UGoKartMovementReplicator::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
}


// Called every frame
void UGoKartMovementReplicator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (MovementComponent)
    {
        ENetRole Role = GetOwnerRole();
        bool bIsLocallyControlled = Cast<APawn>(GetOwner())->IsLocallyControlled();

        FGoKartMove CurrentMove = MovementComponent->GetCurrentMove();
        if (Role == ROLE_AutonomousProxy)
        {
            UnacknowledgedMoves.Add(CurrentMove);
            UpdateServerState(CurrentMove);
        }

        if (Role == ROLE_Authority && bIsLocallyControlled)
        {
            UpdateServerState(CurrentMove);
        }

        if (Role == ROLE_SimulatedProxy)
        {
            ClientTick(DeltaTime);
        }

        DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(Role), GetOwner(), FColor::White, DeltaTime);
    }
}

void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UGoKartMovementReplicator, ServerState);
}

void UGoKartMovementReplicator::UpdateServerState(const FGoKartMove& Move)
{
    if (MovementComponent)
    {
        ServerState.LastMove = Move;
        ServerState.Transform = GetOwner()->GetActorTransform();
        ServerState.Velocity = MovementComponent->GetVelocity();
    }
}

FHermiteCubicSpline UGoKartMovementReplicator::CreateSpline()
{
    FHermiteCubicSpline NewSpline;
    NewSpline.StartLocation = ClientStartTransform.GetLocation();
    NewSpline.TargetLocation = ClientTargetTransform.GetLocation();
    NewSpline.StartDerivative = ClientStartVelocity * VelocityToDerivative;
    NewSpline.TargetDerivative = ServerState.Velocity * VelocityToDerivative;

    return NewSpline;
}

void UGoKartMovementReplicator::InterpolateLocation()
{
    FVector NewLocation = Spline.InterpolateLocation(Alpha);
    if (MeshOffsetRoot)
    {
        MeshOffsetRoot->SetWorldLocation(NewLocation);
    }
}

void UGoKartMovementReplicator::InterpolateVelocity()
{
    FVector NewVelocity = Spline.InterpolateDerivative(Alpha);
    NewVelocity /= VelocityToDerivative;
    MovementComponent->SetVelocity(NewVelocity);
}

void UGoKartMovementReplicator::InterpolateRotation()
{
    FQuat NewRotation = FQuat::Slerp(FQuat(ClientStartTransform.GetRotation()), FQuat(ClientTargetTransform.GetRotation()), Alpha);
    if (MeshOffsetRoot)
    {
        MeshOffsetRoot->SetWorldRotation(NewRotation);
    }
}

void UGoKartMovementReplicator::ClientTick(float DeltaTime)
{
    ClientTimeSinceUpdate += DeltaTime;

    if (ClientTimeBetweenLastUpdates > KINDA_SMALL_NUMBER && MovementComponent)
    {
        ClientTargetTransform = ServerState.Transform;
        Alpha = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;

        Spline = CreateSpline();

        InterpolateLocation();
        InterpolateRotation();
        InterpolateVelocity();
    }
}

void UGoKartMovementReplicator::ClearAcknowledgedMoves(float ServerTime)
{
    if (ServerTime > 0 && UnacknowledgedMoves.Num() > 0)
    {
        for (int32 Index = UnacknowledgedMoves.Num() - 1; Index >= 0 ; --Index)
        {
            if (UnacknowledgedMoves[Index].Time <= ServerTime)
            {
                UnacknowledgedMoves.RemoveAt(Index);
            }
        }
    }
}

void UGoKartMovementReplicator::OnRep_ServerState()
{
    switch (GetOwnerRole())
    {
        case ROLE_AutonomousProxy:
            AutonomousProxy_OnRep_ServerState();
            break;
        case ROLE_SimulatedProxy:
            SimulatedProxy_OnRep_ServerState();
            break;
        default:
            break;
    }
}

void UGoKartMovementReplicator::AutonomousProxy_OnRep_ServerState()
{
    if (MovementComponent)
    {
        GetOwner()->SetActorTransform(ServerState.Transform);
        MovementComponent->SetVelocity(ServerState.Velocity);

        ClearAcknowledgedMoves(ServerState.LastMove.Time);

        for (const FGoKartMove& Move : UnacknowledgedMoves)
        {
            MovementComponent->SimulateMove(Move);
        }
    }
}

void UGoKartMovementReplicator::SimulatedProxy_OnRep_ServerState()
{
    if (MovementComponent)
    {
        ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
        VelocityToDerivative = ClientTimeBetweenLastUpdates * 100;
        ClientTimeSinceUpdate = 0.f;
        ClientStartVelocity = MovementComponent->GetVelocity();

        if (MeshOffsetRoot)
        {
            ClientStartTransform.SetLocation(MeshOffsetRoot->GetComponentLocation());
            ClientStartTransform.SetRotation(MeshOffsetRoot->GetComponentQuat());
        }
        GetOwner()->SetActorTransform(ServerState.Transform);
    }
}