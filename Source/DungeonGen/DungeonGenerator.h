// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Room.h"
class URoomGraphGenerator;
#include "GameFramework/Actor.h"
#include "DungeonGenerator.generated.h"


USTRUCT(BlueprintType)
struct FTriangle2D
{
	GENERATED_BODY()

	UPROPERTY()
	FVector A;

	UPROPERTY()
	FVector B;

	UPROPERTY()
	FVector C;
	
	UPROPERTY()
	FVector2D A2D;

	UPROPERTY()
	FVector2D B2D;

	UPROPERTY()
	FVector2D C2D;
	
	FTriangle2D() {}

	FTriangle2D(const FVector& InA, const FVector& InB, const FVector& InC)
		: A(InA), B(InB), C(InC), A2D(InA.X, InA.Y), B2D(InB.X, InB.Y), C2D(InC.X, InC.Y) {}

	FTriangle2D(const FVector2D& InA, const FVector2D& InB, const FVector2D& InC)
		: A(InA.X, InA.Y, 0), B(InB.X, InB.Y, 0), C(InC.X, InC.Y, 0), A2D(InA), B2D(InB), C2D(InC) {}

	bool operator==(const FTriangle2D& Other) const
	{
		// triangles are equal if all three vertices match (order can matter or not)
		return A == Other.A && B == Other.B && C == Other.C;
	}

};
USTRUCT()
struct FRoomGraphNode
{
	GENERATED_BODY()

	UPROPERTY()
	ARoom* Room;

	// Connected neighbors and weights
	UPROPERTY()
	TArray<ARoom*> Neighbors;

	UPROPERTY()
	TArray<float> Weights; // optional: store weight of each edge

	FRoomGraphNode() : Room(nullptr) {}
	FRoomGraphNode(ARoom* InRoom) : Room(InRoom) {}
};

USTRUCT()
struct FRoomGraphEdge
{
	GENERATED_BODY()

	UPROPERTY()
	ARoom* RoomA;

	UPROPERTY()
	ARoom* RoomB;

	UPROPERTY()
	float Weight;

	FRoomGraphEdge() : RoomA(nullptr), RoomB(nullptr), Weight(0.f) {}
	FRoomGraphEdge(ARoom* InA, ARoom* InB, float InWeight) : RoomA(InA), RoomB(InB), Weight(InWeight) {}
};


USTRUCT()
struct FEdge2D
{
	GENERATED_BODY()

	FVector2D A;
	FVector2D B;

	FEdge2D() {}
	FEdge2D(const FVector2D& InA, const FVector2D& InB) : A(InA), B(InB) {}

	bool operator==(const FEdge2D& Other) const
	{
		return (A == Other.A && B == Other.B) || (A == Other.B && B == Other.A);
	}
};


UCLASS()
class DUNGEONGEN_API ADungeonGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADungeonGenerator();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	URoomGraphGenerator* GraphGenerator;
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void SeparateRoomsStep();
	void StartRoomSeparation();

	static FVector GetRandomPointInCircle(float Size, FVector GenerationCenter);

	UPROPERTY()
	TArray<ARoom*> Rooms;
	UPROPERTY()
	TArray<ARoom*> SelectedRooms;
	UPROPERTY()
	TArray<ARoom*> SelectedCorridorRooms;
	
	bool bAnyOverlap;
	
	FTimerHandle RoomSeparationTimer;

	TArray<FRoomGraphEdge> MST;

	void CreateRooms();
	void SeparateRooms();
	void SelectBiggestRooms(int NumberOfBiggestRooms);
	void GenerateRoomGraph();

	UFUNCTION()
	void BuildCorridorsFromMST(const TArray<FRoomGraphEdge>& InMST);

	void FindIntersectingRooms(const FVector& Start, const FVector& End);
	
public:	
	UPROPERTY(EditAnywhere)
	int RoomsToSpawn;

	UPROPERTY(EditAnywhere)
	int NumberOfBigRoomsToSelect;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* SelectedRoomMaterial;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* SelectedCorridorRoomMaterial;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ARoom> BP_Room;
	
	UPROPERTY(EditAnywhere)
	float RoomSizeMin;
	
	UPROPERTY(EditAnywhere)
	float RoomSizeMax;

	UPROPERTY(EditAnywhere)
	float GenerationRadius;

	UPROPERTY(EditAnywhere)
	float GenerationZ;

	UPROPERTY(EditAnywhere)
	FVector GenerationCenter;
};