#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.h"
#include "GoKartMovementReplicator.generated.h"

USTRUCT()
struct FGoKartState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FGoKartMove LastMove;
	UPROPERTY()
	FVector Velocity;
	UPROPERTY()
	FTransform Transform;

public:
	FORCEINLINE FGoKartMove GetLastMove() {return LastMove;}
};

struct FHermiteCubicSpline
{
	FVector StartLocation, StartDerivative, TargetLocation, TargetDerivative;

	FVector InterpolateLocation(float Alpha) const
	{
		return FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, Alpha);
	}

	FVector InterpolateDerivative(float Alpha) const
	{
		return FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, Alpha);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementReplicator : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementReplicator();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE FGoKartState GetServerState() {return ServerState;}

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	UFUNCTION(BlueprintCallable)
	void SetMeshOffsetRoot(USceneComponent* Root) {MeshOffsetRoot = Root;}

	UFUNCTION()
	void OnRep_ServerState();
	void SimulatedProxy_OnRep_ServerState();
	void AutonomousProxy_OnRep_ServerState();

	void ClearAcknowledgedMoves(float ServerTime);
	
	void UpdateServerState(const FGoKartMove& Move);

	FHermiteCubicSpline CreateSpline();
	void InterpolateLocation();
	void InterpolateVelocity();
	void InterpolateRotation();

	void ClientTick(float DeltaTime);

	UPROPERTY()
	UGoKartMovementComponent* MovementComponent;

	UPROPERTY()
	USceneComponent* MeshOffsetRoot;

	UPROPERTY()
	FGoKartMove LastMove;

	UPROPERTY(ReplicatedUsing=OnRep_ServerState)
	FGoKartState ServerState;

	TArray<FGoKartMove> UnacknowledgedMoves;

	float ClientTimeSinceUpdate;
	float ClientTimeBetweenLastUpdates;
	float VelocityToDerivative;
	FTransform ClientStartTransform;
	FTransform ClientTargetTransform;
	FVector ClientStartVelocity;

	float Alpha;

	FHermiteCubicSpline Spline;
};