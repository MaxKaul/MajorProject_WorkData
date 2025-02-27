// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionDatabase.h"
#include "Bachelor_Test_EnvCharacter.h"
#include "DataManager.h"
#include "MarketManager.h"
#include "GameFramework/Actor.h"
#include "SaveGameManager.generated.h"

struct FAIPlayerSaveData;

UCLASS()
class BACHELOR_TEST_ENV_API ASaveGameManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ASaveGameManager();

	UFUNCTION(BlueprintCallable)
		void SaveGameData();


	UFUNCTION(BlueprintCallable)
		void LoadGameData(FString _saveGameName, int _saveGameSlot);

	UFUNCTION(BlueprintCallable) FORCEINLINE 
		TArray<class AAIPlayer* > GetAllAIPlayer() { return allAIPlayer; }

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

private:
	UFUNCTION()
		bool SpawnAllActor();

	UFUNCTION()
		bool SpawnActionDatabase();
	// Loads the trainingsdata with delay
	UFUNCTION()
		void LoadTrainingsData();

	UFUNCTION()
		bool SpawnDataManager();

	UFUNCTION()
		bool SpawnMarketManager();

	UFUNCTION()
		bool SpawnPredictionSystem();

	UFUNCTION()
		bool SpawnAIPlayer();
	UFUNCTION()
		void InitPlayer();





	UFUNCTION()
		void SaveMarketManagerData();
	UFUNCTION()
		void SavePlayerData();
	UFUNCTION()
		void SaveDataManagerData();
	UFUNCTION()
		void SaveActionDatabaseSaveData();


private:
	UPROPERTY(EditAnywhere,Category = Overrides, meta = (AllowPrivateAccess))
	bool bEnableExponentialSmoothing;

	UPROPERTY(EditAnywhere, Category = SpawnComps, meta = (AllowPrivateAccess))
		int aiSpawnAmount;

	UPROPERTY(EditAnywhere, Category = SpawnComps, meta = (AllowPrivateAccess))
		TSubclassOf<class AAIPlayer> aiPlayerClass;
	UPROPERTY(VisibleAnywhere, Category = SpawnComps, meta = (AllowPrivateAccess))
		TArray<AAIPlayer*> allAIPlayer;
	UPROPERTY(VisibleAnywhere, Category = SpawnComps, meta = (AllowPrivateAccess))
		TArray<FAIPlayerSaveData> allAIPlayer_SaveData;

	UPROPERTY(EditAnywhere, Category = SpawnComps, meta = (AllowPrivateAccess))
		TSubclassOf<class AMarketManager> marketManagerClass;
	UPROPERTY(VisibleAnywhere, Category = SpawnComps, meta = (AllowPrivateAccess))
		AMarketManager* marketManager;

	UPROPERTY(EditAnywhere, Category = SpawnComps, meta = (AllowPrivateAccess))
		TSubclassOf<class ADataManager> dataManagerClass;
	UPROPERTY(VisibleAnywhere, Category = SpawnComps, meta = (AllowPrivateAccess))
		ADataManager* dataManager;

	UPROPERTY(EditAnywhere, Category = SpawnComps, meta = (AllowPrivateAccess))
		TSubclassOf<class AActionDatabase> actionDatabaseClass;
	UPROPERTY(VisibleAnywhere, Category = SpawnComps, meta = (AllowPrivateAccess))
		AActionDatabase* actionDatabase;


	UPROPERTY(EditAnywhere, Category = SpawnComps, meta = (AllowPrivateAccess))
		TSubclassOf<class AMarketPredictionSystem> predictionSystemClass;
	UPROPERTY(VisibleAnywhere, Category = SpawnComps, meta = (AllowPrivateAccess))
		AMarketPredictionSystem* predictionSystem;

	UPROPERTY()
		ABachelor_Test_EnvCharacter* testEnvPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Overrides, meta = (AllowPrivateAccess))
		bool bShouldReconTable;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Overrides, meta = (AllowPrivateAccess))
		int tableAmount;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Overrides, meta = (AllowPrivateAccess))
		int spawnedTables;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Overrides, meta = (AllowPrivateAccess))
		float dataSpawnDelay;

	UPROPERTY()
		FPlayerSaveData playerSaveData;
	UPROPERTY()
		FActionDatabaseSaveData actionDatabaseSave;
	UPROPERTY()
		FDataManagerSaveData dataManagerSaveData;
	UPROPERTY()
		FMarketManagerSaveData marketManagerSaveData;



	UPROPERTY(EditAnywhere, Category = SaveGame, meta = (AllowPrivateAccess))
		FString saveGameName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ManagerSpawns, meta = (AllowPrivateAccess))
		TArray<FString> savedGames;

	UPROPERTY()
		UWorld* world;
};