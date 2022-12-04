// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.generated.h"

USTRUCT()
struct FGoKartMove
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float Throttle;
	UPROPERTY()
	float SteeringThrow;

	UPROPERTY()
	float DeltaTime;
	UPROPERTY()
	float Time;

	bool IsValid() const
	{
		return FMath::Abs(Throttle) <= 1.f && FMath::Abs(SteeringThrow) <= 1.f;
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE FVector GetVelocity() {return Velocity;}
	FORCEINLINE FGoKartMove GetCurrentMove() {return CurrentMove;}

	FORCEINLINE void SetVelocity(FVector InVelocity) {Velocity = InVelocity;}
	FORCEINLINE void SetThrottle(float InThrottle) {Throttle = InThrottle;}
	FORCEINLINE void SetSteeringThrow(float InSteeringThrow) {SteeringThrow = InSteeringThrow;}

	void SimulateMove(const FGoKartMove& Move);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	FGoKartMove CreateMove(float DeltaTime);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SimulateMove(const FGoKartMove& Move);
	
	void UpdateLocationFromVelocity(float DeltaTime);
	void ApplyRotation(float DeltaTime, float InSteeringThrow);
	FVector CalculateAirResistance();
	FVector CalculateRollingResistance();

	// The mass of the car (kg).
	UPROPERTY(EditAnywhere)
	float Mass = 1000.f;

	// The acceleration force applied to the car when the throttle is fully down (N).
	UPROPERTY(EditAnywhere)
	float DrivingForce = 10000;

	// The drag coefficient for calculating the air resistence force.
	UPROPERTY(EditAnywhere, Category = "Movement")
	float DragCoefficient = 16.f;

	// The rolling resistance coefficient for calculating the rolling resistence force.
	UPROPERTY(EditAnywhere, Category = "Movement")
	float RollingResistanceCoefficient = 0.01f;

	// The turning radius of the car.
	UPROPERTY(EditAnywhere, Category = "Movement")
	float TurningRadius = 10.f;

	FGoKartMove CurrentMove;

	FVector Velocity;
	FVector Acceleration;
	float Throttle;
	float SteeringThrow;
	float ClientSimulatedTime;

	UPROPERTY(VisibleAnywhere)
	class UGoKartMovementReplicator* MovementReplicator;
};