// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonGenerator.h"

#include "RoomGraphGenerator.h"


// Sets default values
ADungeonGenerator::ADungeonGenerator()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bAnyOverlap = true;
	GraphGenerator = CreateDefaultSubobject<URoomGraphGenerator>(TEXT("GraphGen"));
	GraphGenerator->OnGraphCompleted.AddDynamic(this, &ADungeonGenerator::BuildCorridorsFromMST);

}

// Called when the game starts or when spawned
void ADungeonGenerator::BeginPlay()
{
	Super::BeginPlay();
	CreateRooms();
	StartRoomSeparation();
}
void ADungeonGenerator::SeparateRoomsStep()
{
	if (!bAnyOverlap)
	{
		// Stop timer
		GetWorldTimerManager().ClearTimer(RoomSeparationTimer);
	
		GenerateRoomGraph();
		return;
	}

	bAnyOverlap = false; // reset for this step
	SeparateRooms();     // this will set bAnyOverlap = true if overlaps are found
}


void ADungeonGenerator::StartRoomSeparation()
{
	bAnyOverlap = true;

	// Start timer that ticks every frame
	GetWorldTimerManager().SetTimer(RoomSeparationTimer, this, &ADungeonGenerator::SeparateRoomsStep, 1/60.0, true);
}


FVector ADungeonGenerator::GetRandomPointInCircle(float Radius, FVector GenerationCenter)
{
	float r = Radius * sqrt(FMath::RandRange(0.0f, 1.0f));
	float theta = (FMath::RandRange(0.0f, 1.0f)) * 2 * PI;

	FVector randomPos = FVector(r * cos(theta), r * sin(theta), 0);
	

	return randomPos + GenerationCenter;
}

// Opti : Avoir un sorted array trié par Area dès la création des salles
void ADungeonGenerator::SelectBiggestRooms(int NumberOfBiggestRooms)
{
	SelectedRooms.Empty();

	// Copy rooms to a temp array to sort
	TArray<ARoom*> SortedRooms = Rooms;

	// Sort descending by area
	SortedRooms.Sort([](const ARoom& A, const ARoom& B)
	{
		return A.Area > B.Area;
	});

	// Take the top NumberOfBiggestRooms
	for (int32 i = 0; i < FMath::Min(NumberOfBiggestRooms, SortedRooms.Num()); ++i)
	{
		SelectedRooms.Add(SortedRooms[i]);
	}

	UE_LOG(LogTemp, Log, TEXT("Selected %d biggest rooms."), SelectedRooms.Num());

	for (ARoom* Room : SelectedRooms)
	{
		Room->mesh->SetMaterial(0, SelectedRoomMaterial);
	}
}
void ADungeonGenerator::GenerateRoomGraph()
{
	for (ARoom* room : Rooms)
	{
		room->ComputeFinalValues();
	}
	// Select 20 biggest rooms
	SelectBiggestRooms(NumberOfBigRoomsToSelect);

	GraphGenerator->GenerateGraph(SelectedRooms);
}

void ADungeonGenerator::CreateRooms()
{
	UE_LOG(LogTemp, Warning, TEXT("%d"), RoomsToSpawn);
	for (int i = 0; i < RoomsToSpawn; i++)
	{
		//spawning params
		FActorSpawnParameters tParams;
		tParams.Owner = this;
		FRotator rot;
		FVector loc = GetRandomPointInCircle(GenerationRadius, GenerationCenter);
		rot = FRotator::ZeroRotator;
		
		// Spawn Actor
		ARoom* newRoom = GetWorld()->SpawnActor<ARoom>(BP_Room, loc, rot, tParams);

		int scaleX = FMath::RandRange(RoomSizeMin, RoomSizeMax);
		int scaleY = FMath::RandRange(RoomSizeMin, RoomSizeMax);

		// Scale room randomly
		FVector scale(scaleX, scaleY,1);

		newRoom->SetActorScale3D(scale);
		newRoom->Area = scaleX * scaleY;
		Rooms.Add(newRoom);
	}
}

void ADungeonGenerator::SeparateRooms()
{
	for (int32 i = 0; i < Rooms.Num(); ++i)
	{
		for (int32 j = i + 1; j < Rooms.Num(); ++j)
		{
			ARoom* RoomA = Rooms[i];
			ARoom* RoomB = Rooms[j];

			if (!RoomA || !RoomB) continue;

			if (RoomA->IsOverlappingActor(RoomB))
			{

				// Get actor bounds (center and extent)
				FVector OriginA, ExtentA;
				RoomA->GetActorBounds(true, OriginA, ExtentA);

				FVector OriginB, ExtentB;
				RoomB->GetActorBounds(true, OriginB, ExtentB);

				// Compute the overlap in X and Y
				FVector Delta = OriginB - OriginA;
				FVector Overlap;

				Overlap.X = ExtentA.X + ExtentB.X - FMath::Abs(Delta.X);
				Overlap.Y = ExtentA.Y + ExtentB.Y - FMath::Abs(Delta.Y);
				Overlap.Z = 0; // Assume top-down 2D layout
				
					
				// Only separate if overlap is positive and above a certain threshold or else it will never converge
				if (Overlap.X > 0 && Overlap.Y > 0)
				{
					bAnyOverlap = true;
					// Move in the axis with less overlap
					FVector Separation = FVector::ZeroVector;

					if (Overlap.X < Overlap.Y)
					{
						// add +0.05 to really separate rooms so they're not adjacent (else it still trigger overlap, and also converges faster)
						Separation.X = (Delta.X < 0 ? -1 : 1) * (Overlap.X * 0.5f + 0.1f);
					}
					else
					{
						Separation.Y = (Delta.Y < 0 ? -1 : 1) * (Overlap.Y * 0.5f + 0.1f);
					}

					RoomA->AddActorWorldOffset(-Separation);
					RoomB->AddActorWorldOffset(Separation);
				}
			}
		}
	}
}

void ADungeonGenerator::BuildCorridorsFromMST(const TArray<FRoomGraphEdge>& InMST)
{
	MST = InMST;
    UWorld* World = GetWorld();
    if (!World) return;

    for (const FRoomGraphEdge& Edge : MST)
    {
        if (!Edge.RoomA || !Edge.RoomB) continue;
        FVector PosA = Edge.RoomA->GetCenter(); 
        FVector PosB = Edge.RoomB->GetCenter();

        // Get extents
        float HalfWidthA = Edge.RoomA->GetWidth() * 0.5f;
        float HalfWidthB = Edge.RoomB->GetWidth() * 0.5f;
        float HalfHeightA = Edge.RoomA->GetHeight() * 0.5f;
        float HalfHeightB = Edge.RoomB->GetHeight() * 0.5f;

        float MinAX = PosA.X - HalfWidthA;
        float MaxAX = PosA.X + HalfWidthA;
        float MinBX = PosB.X - HalfWidthB;
        float MaxBX = PosB.X + HalfWidthB;

        float MinAY = PosA.Y - HalfHeightA;
        float MaxAY = PosA.Y + HalfHeightA;
        float MinBY = PosB.Y - HalfHeightB;
        float MaxBY = PosB.Y + HalfHeightB;

        bool bOverlapX = (MinAX <= MaxBX) && (MaxAX >= MinBX);
        bool bOverlapY = (MinAY <= MaxBY) && (MaxAY >= MinBY);

        if (bOverlapX)
        {
            // Draw vertical corridor: align X in the middle
            float MidX = FMath::Max(MinAX, MinBX) + (FMath::Min(MaxAX, MaxBX) - FMath::Max(MinAX, MinBX)) * 0.5f;
            FVector From(MidX, PosA.Y, PosA.Z);
            FVector To(MidX, PosB.Y, PosB.Z);
            DrawDebugLine(World, From, To, FColor::Blue, true, 10.f, 0, 50.f);
        }
        else if (bOverlapY)
        {
            // Draw horizontal corridor: align Y in the middle
            float MidY = FMath::Max(MinAY, MinBY) + (FMath::Min(MaxAY, MaxBY) - FMath::Max(MinAY, MinBY)) * 0.5f;
            FVector From(PosA.X, MidY, PosA.Z);
            FVector To(PosB.X, MidY, PosB.Z);
            DrawDebugLine(World, From, To, FColor::Blue, true, 10.f, 0, 50.f);
			FindIntersectingRooms(From, To);
        }
        else
        {
            // L-shaped: first go in X, then in Y (you could randomize the order)
            FVector Corner(PosB.X, PosA.Y, PosA.Z);
            DrawDebugLine(World, PosA, Corner, FColor::Blue, true, 10.f, 0, 50.f);
            DrawDebugLine(World, Corner, PosB, FColor::Blue, true, 10.f, 0, 50.f);
        	FindIntersectingRooms(PosA, Corner);
        	FindIntersectingRooms(PosB, Corner);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Corridors drawn from MST."));
	
	// TODO NEXT STEPS : DELETE UNSELECTED ROOMS, BUILD REAL CORRIDORS, REAL ROOMS AND CONNECTION MODULES WITH DOORS
}

void ADungeonGenerator::FindIntersectingRooms(const FVector& Start, const FVector& End)
{
	TArray<FHitResult> HitResults;

	FCollisionQueryParams TraceParams;
	// Ignore the selected rooms:
	for (ARoom* Selected : SelectedRooms)
	{
		if (Selected)
			TraceParams.AddIgnoredActor(Selected);
	}

	GetWorld()->LineTraceMultiByChannel(
		HitResults,
		Start,
		End,
		ECC_WorldStatic,   // or ECC_GameTraceChannel1 if you set up a custom channel
		TraceParams
	);
	for (const FHitResult& Hit : HitResults)
	{
		ARoom* HitRoom = Cast<ARoom>(Hit.GetActor());
		if (HitRoom && !SelectedRooms.Contains(HitRoom) && !SelectedCorridorRooms.Contains(HitRoom))
		{
			SelectedCorridorRooms.Add(HitRoom);
			// Change its material
			UStaticMeshComponent* Mesh = HitRoom->FindComponentByClass<UStaticMeshComponent>();
			if (Mesh)
			{
				Mesh->SetMaterial(0, SelectedCorridorRoomMaterial);
			}
		}
	}
}