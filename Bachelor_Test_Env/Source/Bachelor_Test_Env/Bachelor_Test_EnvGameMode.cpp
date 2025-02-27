// Copyright Epic Games, Inc. All Rights Reserved.

#include "Bachelor_Test_EnvGameMode.h"
#include "Bachelor_Test_EnvPlayerController.h"
#include "Bachelor_Test_EnvCharacter.h"
#include "UObject/ConstructorHelpers.h"

ABachelor_Test_EnvGameMode::ABachelor_Test_EnvGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = ABachelor_Test_EnvPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownCharacter"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	// set default controller to our Blueprinted controller
	static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerBPClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownPlayerController"));
	if(PlayerControllerBPClass.Class != NULL)
	{
		PlayerControllerClass = PlayerControllerBPClass.Class;
	}
}