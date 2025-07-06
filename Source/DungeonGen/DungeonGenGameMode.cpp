// Copyright Epic Games, Inc. All Rights Reserved.

#include "DungeonGenGameMode.h"
#include "DungeonGenCharacter.h"
#include "UObject/ConstructorHelpers.h"

ADungeonGenGameMode::ADungeonGenGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
