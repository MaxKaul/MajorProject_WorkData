// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnumLibrary.h"
#include "GameFramework/Character.h"
#include "Bachelor_Test_EnvCharacter.generated.h"


USTRUCT()
struct FPlayerSaveData
{
	GENERATED_BODY()

	FPlayerSaveData() {}


	FPlayerSaveData(TMap<EResourceIdent, int> _resourcePouch)
	{
		s_resourcePouch = _resourcePouch;

		if (s_resourcePouch.Num() >= 0)
			s_bSaveWasInit = true;
	}

private:
	UPROPERTY()
		TMap<EResourceIdent, int> s_resourcePouch;
	UPROPERTY()
		bool s_bSaveWasInit;

public:
	FORCEINLINE
		TMap<EResourceIdent, int> S_GetSavedPoolInfo() { return s_resourcePouch; }

	FORCEINLINE
		bool S_WasSaveInit() { return s_bSaveWasInit; }
};


UCLASS(Blueprintable)
class ABachelor_Test_EnvCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABachelor_Test_EnvCharacter();

	virtual void Tick(float DeltaSeconds) override;

	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }


	UFUNCTION()
		void InitPlayer(FPlayerSaveData _saveData);

	UFUNCTION()
		FPlayerSaveData GetPlayerSaveData();

	UFUNCTION(BlueprintCallable)
		void TESTBUY();
	UFUNCTION(BlueprintCallable)
		void TESTSell();

	UPROPERTY(EditAnywhere)
		TSubclassOf<class AMarketManager> marketManagerClass;

	FORCEINLINE
		USpringArmComponent* GetCameraBoom() { return CameraBoom_0; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom_0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY(EditAnywhere, Category = PlayerInfo, meta = (AllowPrivateAccess))
		TMap<EResourceIdent, int> resourcePouch;
};

