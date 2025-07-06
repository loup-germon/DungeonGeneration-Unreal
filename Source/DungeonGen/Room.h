// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Room.generated.h"

UCLASS()
class DUNGEONGEN_API ARoom : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARoom();

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* mesh;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	FVector Center;
	FVector2D Center2D;
	float Width;
	float Height;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	bool OverlapsWithOtherRoom(TArray<AActor*>& OverlappingActors);
	float Area;
	void SetArea(float InArea);

	void ComputeFinalValues();
	
	FVector GetCenter();
	FVector2D GetCenter2D();
	float GetWidth();
	float GetHeight();
};
