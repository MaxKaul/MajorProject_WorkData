// Copyright Epic Games, Inc. All Rights Reserved.

#include "Bachelor_Test_EnvCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "MarketManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ABachelor_Test_EnvCharacter::ABachelor_Test_EnvCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera boom...
	CameraBoom_0 = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom_0"));
	CameraBoom_0->SetupAttachment(RootComponent);
	CameraBoom_0->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom_0->TargetArmLength = 800.f;
	CameraBoom_0->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom_0->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom_0, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ABachelor_Test_EnvCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

void ABachelor_Test_EnvCharacter::InitPlayer(FPlayerSaveData _saveData)
{
	if(!_saveData.S_WasSaveInit())
		return;

	for(TTuple<EResourceIdent, int> saveitem : _saveData.S_GetSavedPoolInfo())
	{
		resourcePouch.Add(saveitem.Key, saveitem.Value);
	}
}

FPlayerSaveData ABachelor_Test_EnvCharacter::GetPlayerSaveData()
{
	FPlayerSaveData newsave =
	{
		resourcePouch
	};

	return newsave;
}

void ABachelor_Test_EnvCharacter::TESTBUY()
{
	AMarketManager* mm = Cast<AMarketManager>(UGameplayStatics::GetActorOfClass(GetWorld(), marketManagerClass));

	TArray<FResourceTransactionTicket> tickets;

	FResourceTransactionTicket testticket =
	{
		EResourceIdent::RI_Water,
		10,
		mm->GetResourcePoolInfo().FindRef(EResourceIdent::RI_Water).GetModifiedPrice() * 10
	};

	tickets.Add(testticket);

	mm->BuyResources(tickets);
}

void ABachelor_Test_EnvCharacter::TESTSell()
{
	AMarketManager* mm = Cast<AMarketManager>(UGameplayStatics::GetActorOfClass(GetWorld(), marketManagerClass));

	TArray<FResourceTransactionTicket> tickets;

	FResourceTransactionTicket testticket =
	{
		EResourceIdent::RI_Stone,
		10,
		0
	};

	tickets.Add(testticket);

	mm->SellResources(tickets);
}
