// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DungeonGenerator.h"
#include "Components/ActorComponent.h"
#include "RoomGraphGenerator.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProcessFinished, const TArray<FRoomGraphEdge>&, MST);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DUNGEONGEN_API URoomGraphGenerator : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URoomGraphGenerator();

protected:

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY()
	TArray<ARoom*> Rooms;
	UPROPERTY()
	TArray<ARoom*> SelectedRooms;
	
	UPROPERTY(BlueprintAssignable)
	FOnProcessFinished OnGraphCompleted;

	float DelayBetweenSteps;

	UFUNCTION()
	void GenerateGraph(const TArray<ARoom*>& InSelectedRooms);
	TArray<FRoomGraphEdge> MST;
	TArray<FTriangle2D> Triangles;
	FTriangle2D SuperTriangle;
	UPROPERTY()
	TMap<ARoom*, FRoomGraphNode> RoomGraph;
	
	
	FTriangle2D ComputeSuperTriangle();
	void DrawTriangle(const FTriangle2D& Triangle);
	void DrawAllTriangles();
	void ComputeCircumscribedCircle2D(const FTriangle2D& Triangle, FVector2D& OutCenter, float& OutRadius);
	void DelaunayStep(FVector2d Point);
	void PerformDelaunayTriangulation();
	void BuildRoomGraphFromTriangulation();
	void ComputeMinimumSpanningTree();

	FTimerHandle DelayTimerHandle;
};
