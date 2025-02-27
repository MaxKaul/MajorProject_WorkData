// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionDatabase.h"
#include "AIPlayer.h"
#include "Bachelor_Test_EnvCharacter.h"
#include "DataManager.h"
#include "MarketManager.h"
#include "GameFramework/SaveGame.h"
#include "BTE_SaveGame.generated.h"



UCLASS()
class BACHELOR_TEST_ENV_API UBTE_SaveGame : public USaveGame
{
	GENERATED_BODY()


public:
	UFUNCTION()
		void InitSaveGame(FMarketManagerSaveData _marketManagerSaveData, TArray<FAIPlayerSaveData> _allAIPlayerSaveData, FPlayerSaveData _playerSaveData,
						  FActionDatabaseSaveData _actionDatabaseSaveData, FDataManagerSaveData _dataManagersaveData);

	FORCEINLINE
		FMarketManagerSaveData GetMarketManagerSaveData() { return marketManagerSaveData; }
	FORCEINLINE
		TArray<FAIPlayerSaveData> GetAllAIPlayerSaveData() { return allAIPlayerSaveData; }
	FORCEINLINE
		FPlayerSaveData GetPlayerSaveData() { return playerSaveData; }
	FORCEINLINE
		FActionDatabaseSaveData GetActionDatabaseSaveData() { return actionDatabaseSaveData; }
	FORCEINLINE
		FDataManagerSaveData GetDataManagerSaveData() { return dataManagersaveData; }

private:
	UPROPERTY(VisibleAnywhere, Category = SaveGameInfo, meta = (AllowPrivateAccess))
		FMarketManagerSaveData marketManagerSaveData;
	UPROPERTY(VisibleAnywhere, Category = SaveGameInfo, meta = (AllowPrivateAccess))
		TArray<FAIPlayerSaveData> allAIPlayerSaveData;
	UPROPERTY(VisibleAnywhere, Category = SaveGameInfo, meta = (AllowPrivateAccess))
		FPlayerSaveData playerSaveData;
	UPROPERTY(VisibleAnywhere, Category = SaveGameInfo, meta = (AllowPrivateAccess))
		FActionDatabaseSaveData actionDatabaseSaveData;
	UPROPERTY(VisibleAnywhere, Category = SaveGameInfo, meta = (AllowPrivateAccess))
		FDataManagerSaveData dataManagersaveData;
};
