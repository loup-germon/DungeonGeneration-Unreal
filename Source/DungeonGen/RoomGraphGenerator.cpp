// Fill out your copyright notice in the Description page of Project Settings.


#include "RoomGraphGenerator.h"

// Sets default values for this component's properties
URoomGraphGenerator::URoomGraphGenerator()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	DelayBetweenSteps = 1.5f;
}

void URoomGraphGenerator::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	DrawAllTriangles();
}

void URoomGraphGenerator::GenerateGraph(const TArray<ARoom*>& InSelectedRooms)
{
	SelectedRooms = InSelectedRooms;

	PerformDelaunayTriangulation();
}

// Opti possible : juste prendre un super grand triangle au lieu de faire ça
FTriangle2D URoomGraphGenerator::ComputeSuperTriangle()
{
	// Compute overall bounding box
	FVector Min(FLT_MAX, FLT_MAX, 0);
	FVector Max(-FLT_MAX, -FLT_MAX, 0);

	for (ARoom* Room : SelectedRooms)
	{
		if (!Room) continue;

		FVector Origin, Extent;
		Room->GetActorBounds(true, Origin, Extent);

		FVector RoomMin = Origin - Extent;
		FVector RoomMax = Origin + Extent;

		Min.X = FMath::Min(Min.X, RoomMin.X);
		Min.Y = FMath::Min(Min.Y, RoomMin.Y);

		Max.X = FMath::Max(Max.X, RoomMax.X);
		Max.Y = FMath::Max(Max.Y, RoomMax.Y);
	}

	// Expand bounding box slightly (10%)
	float MarginX = (Max.X - Min.X) * 0.1f;
	float MarginY = (Max.Y - Min.Y) * 0.1f;

	Min.X -= MarginX;
	Min.Y -= MarginY;
	Max.X += MarginX;
	Max.Y += MarginY;

	// Build a big triangle that fully contains the bounding box
	// Create a triangle above and wide enough
	FVector P1(Min.X - (Max.X - Min.X), Min.Y - (Max.Y - Min.Y), 0); // bottom-left far
	FVector P2(Max.X + (Max.X - Min.X), Min.Y - (Max.Y - Min.Y), 0); // bottom-right far
	FVector P3((Min.X + Max.X) / 2, Max.Y + (Max.Y - Min.Y) * 2.0f, 0); // top-center far
	

	return FTriangle2D(P1, P2, P3);
}
void URoomGraphGenerator::DrawTriangle(const FTriangle2D& Triangle)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FColor LineColor = FColor::Green;
		float Duration = 0.0f; // seconds, set to 0 for one frame
		bool bPersistent = false;
		float Thickness = 20.0f;

		// Draw lines between the three points to form the triangle
		DrawDebugLine(World, Triangle.A, Triangle.B, LineColor, bPersistent, Duration, 0, Thickness);
		DrawDebugLine(World, Triangle.B, Triangle.C, LineColor, bPersistent, Duration, 0, Thickness);
		DrawDebugLine(World, Triangle.C, Triangle.A, LineColor, bPersistent, Duration, 0, Thickness);
	}
}


void URoomGraphGenerator::DrawAllTriangles()
{
	for (auto Triangle: Triangles)
	{
		DrawTriangle(Triangle);
	}
}
void URoomGraphGenerator::ComputeCircumscribedCircle2D(const FTriangle2D& Triangle, FVector2D& OutCenter, float& OutRadius)
{
	// Project points to 2D (XY plane)
	FVector2D A = Triangle.A2D;
	FVector2D B = Triangle.B2D;
	FVector2D C = Triangle.C2D;;

	// Calculate midpoints
	FVector2D MidAB = (A + B) * 0.5f;
	FVector2D MidBC = (B + C) * 0.5f;

	// Calculate perpendicular directions
	FVector2D DirAB = B - A;
	FVector2D DirBC = C - B;

	FVector2D PerpAB(-DirAB.Y, DirAB.X);
	FVector2D PerpBC(-DirBC.Y, DirBC.X);

	// Solve intersection: MidAB + PerpAB * t = MidBC + PerpBC * s
	float Denom = PerpAB.X * PerpBC.Y - PerpAB.Y * PerpBC.X;

	if (FMath::IsNearlyZero(Denom))
	{
		// Triangle is degenerate (points colinear): use average
		OutCenter = (A + B + C) / 3.0f;
		OutRadius = 0.0f;
		return;
	}

	FVector2D Delta = MidBC - MidAB;
	float t = (Delta.X * PerpBC.Y - Delta.Y * PerpBC.X) / Denom;

	OutCenter = MidAB + PerpAB * t;

	// Compute radius
	OutRadius = FVector2D::Distance(OutCenter, A);

	DrawDebugCircle(GetWorld(),
		FVector(OutCenter.X, OutCenter.Y, 0.0f), OutRadius, 100,
		FColor::Red, false, 1,  0, 3,
		FVector(1, 0, 0), FVector(0, 1, 0), false);
	
}
void URoomGraphGenerator::DelaunayStep(FVector2d Point)
{
	TArray<FTriangle2D> BadTriangles;

	// Find bad triangles
	for (const FTriangle2D& Tri : Triangles)
	{
		FVector2D Center;
		float Radius;
		ComputeCircumscribedCircle2D(Tri, Center, Radius);

		if (FVector2D::Distance(Center, Point) < Radius)
		{
			BadTriangles.Add(Tri);
		}
	}

	// Find boundary (edges that are unique)
	TArray<FEdge2D> Polygon;

	for (const FTriangle2D& BadTri : BadTriangles)
	{
		TArray<FEdge2D> Edges = {
			FEdge2D(BadTri.A2D, BadTri.B2D),
			FEdge2D(BadTri.B2D, BadTri.C2D),
			FEdge2D(BadTri.C2D, BadTri.A2D)
		};

		for (const FEdge2D& Edge : Edges)
		{
			bool bIsShared = false;
			for (int32 i = 0; i < Polygon.Num(); ++i)
			{
				if (Polygon[i] == Edge)
				{
					Polygon.RemoveAt(i);
					bIsShared = true;
					break;
				}
			}
			if (!bIsShared)
			{
				Polygon.Add(Edge);
			}
		}
	}

	// Remove bad triangles
	for (const FTriangle2D& BadTri : BadTriangles)
	{
		Triangles.RemoveSingle(BadTri);
	}

	// Create new triangles
	for (const FEdge2D& Edge : Polygon)
	{
		Triangles.Add(FTriangle2D(Edge.A, Edge.B, Point));
	}
}
void URoomGraphGenerator::PerformDelaunayTriangulation()
{
	SuperTriangle = ComputeSuperTriangle();
	
	Triangles.Add(SuperTriangle);
	FVector2D CenterSuperTriangle;
	float RadiusSuperTriangle;
	ComputeCircumscribedCircle2D(Triangles[0], CenterSuperTriangle, RadiusSuperTriangle);

	
	// Step 3: Prepare points from SelectedRooms
	TArray<FVector2D> Points;
	for (ARoom* Room : SelectedRooms)
	{
		if (Room)
		{
			FVector Loc = Room->GetCenter();
			Points.Add(FVector2D(Loc.X, Loc.Y));
		}
	}

	
	for (const FVector2D& Point : Points)
    {
        DelaunayStep(Point);
    }

    // Step 5: Remove triangles containing super-triangle vertices
    for (int32 i = Triangles.Num() - 1; i >= 0; --i)
    {
        const FTriangle2D& Tri = Triangles[i];

        if (Tri.A2D == SuperTriangle.A2D ||
            Tri.A2D == SuperTriangle.B2D ||
            Tri.A2D == SuperTriangle.C2D ||
            Tri.B2D == SuperTriangle.A2D ||
            Tri.B2D == SuperTriangle.B2D ||
            Tri.B2D == SuperTriangle.C2D ||
            Tri.C2D == SuperTriangle.A2D ||
            Tri.C2D == SuperTriangle.B2D ||
            Tri.C2D == SuperTriangle.C2D)
        {
            Triangles.RemoveAt(i);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Delaunay triangulation completed. %d triangles created."), Triangles.Num());

	// Start a timer to call BuildRoomGraphFromTriangulation after x seconds
	GetOwner()->GetWorldTimerManager().SetTimer(
		DelayTimerHandle,
		this,
		&URoomGraphGenerator::BuildRoomGraphFromTriangulation,
		DelayBetweenSteps,
		false
	);
}

// Constructs the graph structure from the list of triangles
// Opti possible : avoir des ref de Room dans mes triangles pour éviter d'avoir à reconstruire tout à partir des pos
void URoomGraphGenerator::BuildRoomGraphFromTriangulation()
{
	RoomGraph.Empty();

	// Create graph nodes first
	for (ARoom* Room : SelectedRooms)
	{
		if (Room)
		{
			RoomGraph.Add(Room, FRoomGraphNode(Room));
		}
	}

	// Helper: map FVector2D -> ARoom*
	TMap<FVector2D, ARoom*> PointToRoom;
	for (ARoom* Room : SelectedRooms)
	{
		PointToRoom.Add(Room->GetCenter2D(), Room);
	}

	// For each triangle, add edges between its corners
	for (const FTriangle2D& Tri : Triangles)
	{
		TArray<FVector2D> Corners = { Tri.A2D, Tri.B2D, Tri.C2D };

		for (int i = 0; i < 3; ++i)
		{
			ARoom** RoomA = PointToRoom.Find(Corners[i]);
			ARoom** RoomB = PointToRoom.Find(Corners[(i + 1) % 3]);

			if (RoomA && RoomB && *RoomA != *RoomB)
			{
				// Compute weight (distance)
				float Dist = FVector::Dist(
					(*RoomA)->GetCenter(),
					(*RoomB)->GetCenter());

				// Add neighbor if not already connected
				FRoomGraphNode& NodeA = RoomGraph[*RoomA];
				if (!NodeA.Neighbors.Contains(*RoomB))
				{
					NodeA.Neighbors.Add(*RoomB);
					NodeA.Weights.Add(Dist);
				}

				FRoomGraphNode& NodeB = RoomGraph[*RoomB];
				if (!NodeB.Neighbors.Contains(*RoomA))
				{
					NodeB.Neighbors.Add(*RoomA);
					NodeB.Weights.Add(Dist);
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Room graph built. Nodes: %d"), RoomGraph.Num());

	// Start a timer to call ComputeMinimumSpanningTree after x seconds
	GetOwner()->GetWorldTimerManager().SetTimer(
		DelayTimerHandle,
		this,
		&URoomGraphGenerator::ComputeMinimumSpanningTree,
		DelayBetweenSteps,
		false
	);
}

void URoomGraphGenerator::ComputeMinimumSpanningTree()
{
	MST.Empty();
    if (RoomGraph.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Room graph is empty."));
        return;
    }

    // Step 1: Choose arbitrary starting room
    TSet<ARoom*> Visited;
    TArray<ARoom*> AllRooms;
    RoomGraph.GetKeys(AllRooms);
    ARoom* StartRoom = AllRooms[0];
    Visited.Add(StartRoom);

    // Step 2: Candidate edges (neighbors of the starting room)
    struct FEdgeCandidate
    {
        ARoom* From;
        ARoom* To;
        float Weight;

        FEdgeCandidate(ARoom* InFrom, ARoom* InTo, float InWeight)
            : From(InFrom), To(InTo), Weight(InWeight) {}
    };

    TArray<FEdgeCandidate> EdgeCandidates;

    // Add initial edges from StartRoom
    const FRoomGraphNode& StartNode = RoomGraph[StartRoom];
    for (int32 i = 0; i < StartNode.Neighbors.Num(); ++i)
    {
        EdgeCandidates.Add(FEdgeCandidate(StartRoom, StartNode.Neighbors[i], StartNode.Weights[i]));
    }

    // Step 3: Build MST
    while (Visited.Num() < RoomGraph.Num() && EdgeCandidates.Num() > 0)
    {
        // Find edge with smallest weight
        int32 BestIndex = 0;
        float BestWeight = EdgeCandidates[0].Weight;

        for (int32 i = 1; i < EdgeCandidates.Num(); ++i)
        {
            if (EdgeCandidates[i].Weight < BestWeight)
            {
                BestIndex = i;
                BestWeight = EdgeCandidates[i].Weight;
            }
        }

        FEdgeCandidate BestEdge = EdgeCandidates[BestIndex];
        EdgeCandidates.RemoveAt(BestIndex);

        // If the destination room is already visited, skip
        if (Visited.Contains(BestEdge.To))
        {
            continue;
        }

        // Add edge to MST
        MST.Add(FRoomGraphEdge(BestEdge.From, BestEdge.To, BestEdge.Weight));
        Visited.Add(BestEdge.To);

        // Add new edges from the newly added room
        const FRoomGraphNode& NewNode = RoomGraph[BestEdge.To];
        for (int32 i = 0; i < NewNode.Neighbors.Num(); ++i)
        {
            ARoom* Neighbor = NewNode.Neighbors[i];
            if (!Visited.Contains(Neighbor))
            {
                EdgeCandidates.Add(FEdgeCandidate(BestEdge.To, Neighbor, NewNode.Weights[i]));
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MST built with %d edges."), MST.Num());
	for (const FRoomGraphEdge& Edge : MST)
	{
		if (Edge.RoomA && Edge.RoomB)
		{
			FVector A = Edge.RoomA->GetCenter();
			FVector B = Edge.RoomB->GetCenter();

			DrawDebugLine(GetWorld(), A, B, FColor::Red, true, 10.0f, 0, 35.0f);
		}
	}

	// Notify the owner that this step is complete
	OnGraphCompleted.Broadcast(MST);
}


