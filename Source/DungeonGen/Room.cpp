// Fill out your copyright notice in the Description page of Project Settings.


#include "Room.h"

#include "MovieSceneSequenceID.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ARoom::ARoom()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(mesh);
}

// Called when the game starts or when spawned
void ARoom::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ARoom::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool ARoom::OverlapsWithOtherRoom(TArray<AActor*>& OverlappingActors)
{

	// Make sure collision settings allow overlap queries
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this); // Ignore self
	
	return UKismetSystemLibrary::ComponentOverlapActors(mesh, mesh->GetComponentTransform(), ObjectTypes, nullptr
		, IgnoreActors, OverlappingActors);
}

void ARoom::SetArea(float InArea)
{
	Area = InArea;
}

void ARoom::ComputeFinalValues()
{
	FVector Origin, Extent;
	this->GetActorBounds(true, Origin, Extent);
	Center = Origin;
	Center2D = FVector2D(Center.X, Center.Y);
	Width = Extent.X;
	Height = Extent.Y;
}

FVector ARoom::GetCenter()
{
	return Center;
}

FVector2D ARoom::GetCenter2D()
{
	return Center2D;
}

float ARoom::GetWidth()
{
	return Width;
}
float ARoom::GetHeight()
{
	return Height;
}