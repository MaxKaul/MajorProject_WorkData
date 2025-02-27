// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveGameManager.h"

#include "AIPlayer.h"
#include "BTE_SaveGame.h"
#include "MarketManager.h"
#include "MarketPredictionSystem.h"
#include "ActionDatabase.h"
#include "DataManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetStringLibrary.h"

ASaveGameManager::ASaveGameManager()
{
	PrimaryActorTick.bCanEverTick = true;

}

void ASaveGameManager::BeginPlay()
{
	Super::BeginPlay();

	world = GetWorld();


	LoadGameData("save_0", 0);

	if(!SpawnActionDatabase())
		UE_LOG(LogTemp,Error,TEXT("SpawnActionDatabase, Missing Depends"))
}

void ASaveGameManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool ASaveGameManager::SpawnAllActor()
{
	if (!SpawnDataManager()		|| 
		!SpawnMarketManager()	|| 
		!SpawnAIPlayer()		||
		!SpawnPredictionSystem())
		return false;


	InitPlayer();

	return true;
}

bool ASaveGameManager::SpawnActionDatabase()
{
	if (!actionDatabaseClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASaveGameManager, !actionDatabaseClass"));
		return false;
	}

	actionDatabase = world->SpawnActor<AActionDatabase>(actionDatabaseClass);
	actionDatabase->InitActionDatabase(actionDatabaseSave, this, bEnableExponentialSmoothing);

	if (bShouldReconTable)
		LoadTrainingsData();

	return true;
}

void ASaveGameManager::LoadTrainingsData()
{
	FString dir = UKismetSystemLibrary::GetProjectDirectory();

	dir += "\\Content";
	dir += "\\MarketData";

	FString filename = UKismetStringLibrary::Conv_IntToString(spawnedTables);

	actionDatabase->LoadCSVFromFile(dir, filename, tableAmount);

	spawnedTables++;

	UE_LOG(LogTemp, Warning, TEXT("Number of loaded test data: %i / %i"), spawnedTables, tableAmount);


	if (spawnedTables < tableAmount)
	{
		FTimerHandle handle;
		world->GetTimerManager().SetTimer(handle, this, &ASaveGameManager::LoadTrainingsData, dataSpawnDelay, false);
		return;
	}

	if(!SpawnAllActor())
		UE_LOG(LogTemp,Warning,TEXT("SpawnAllActor, LoadTrainingsData, MissingDepends"))
}


bool ASaveGameManager::SpawnDataManager()
{
	if (!dataManagerClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASaveGameManager, !dataManagerClass"));
		return false;
	}

	dataManager = world->SpawnActor<ADataManager>(dataManagerClass);
	dataManager->InitDataManager(dataManagerSaveData ,actionDatabase);

	return true;
}

bool ASaveGameManager::SpawnMarketManager()
{
	if(!marketManagerClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASaveGameManager, !marketManagerClass"));
		return false;
	}

	marketManager = world->SpawnActor<AMarketManager>(marketManagerClass);
	marketManager->InitMarketManager(marketManagerSaveData, dataManager, actionDatabase, this);

	return true;
}

bool ASaveGameManager::SpawnPredictionSystem()
{
	if (bEnableExponentialSmoothing)
		return true;

	if (!predictionSystemClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASaveGameManager, !predictionSystem"));
		return false;
	}

	predictionSystem = world->SpawnActor<AMarketPredictionSystem>(predictionSystemClass);
	predictionSystem->InitPredictionSystem(actionDatabase);

	return true;
}

bool ASaveGameManager::SpawnAIPlayer()
{
	if (!aiPlayerClass || !marketManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASaveGameManager, !aiPlayerClass || !marketManager"));
		return false;
	}


	if(allAIPlayer_SaveData.Num() <= 0)
	{

		for (size_t i = 0; i < aiSpawnAmount; i++)
		{
			AAIPlayer* aiplayer = world->SpawnActor<AAIPlayer>(aiPlayerClass);

			if (!aiplayer)
				continue;

			aiplayer->InitAIPlayer(i, FAIPlayerSaveData(), marketManager, dataManager);
			allAIPlayer.Add(aiplayer);
		}

		return true;
	}

	for(FAIPlayerSaveData savedata : allAIPlayer_SaveData)
	{
		AAIPlayer* aiplayer = world->SpawnActor<AAIPlayer>(aiPlayerClass);

		if (!aiplayer)
			continue;

		aiplayer->InitAIPlayer(savedata.S_GetAiPlayerID(), savedata, marketManager, dataManager);
		allAIPlayer.Add(aiplayer);
	}

	return true;
}

void ASaveGameManager::InitPlayer()
{
	testEnvPlayer = Cast<ABachelor_Test_EnvCharacter>(UGameplayStatics::GetPlayerCharacter(world, 0));

	if (!playerSaveData.S_WasSaveInit())
		return;

	testEnvPlayer->InitPlayer(playerSaveData);
}

void ASaveGameManager::SaveMarketManagerData()
{
	marketManagerSaveData = marketManager->GetMarketManagerSaveData();
}

void ASaveGameManager::SavePlayerData()
{
	for (AAIPlayer* player : allAIPlayer)
	{
		allAIPlayer_SaveData.Add(player->GetAIPlayerSaveData());
	}

	playerSaveData = testEnvPlayer->GetPlayerSaveData();
}

void ASaveGameManager::SaveDataManagerData()
{
	dataManagerSaveData = dataManager->GetDataManagerSaveData();
}

void ASaveGameManager::SaveActionDatabaseSaveData()
{
	actionDatabaseSave = actionDatabase->GetActionDataBaseSaveData();
}

void ASaveGameManager::SaveGameData()
{
	// null check incase of wrong world initialisation
	if (!marketManager || allAIPlayer.Num() <= 0 || !testEnvPlayer)
	{
		UE_LOG(LogTemp,Warning,TEXT("ASaveGameManager, SaveGameData missing depends"));
		return;
	}

	// Get save directory and check for the amount of savegames to number them
	FString savedir = FPaths::ProjectSavedDir() + TEXT("SaveGames/");
	IFileManager::Get().FindFiles(savedGames, *savedir, TEXT("*.sav"));
	if (saveGameName == "")
		saveGameName = FString("save_") + FString::FromInt(savedGames.Num());

	// Call all systems to save their data
	SaveMarketManagerData();
	SavePlayerData();
	SaveActionDatabaseSaveData();
	SaveDataManagerData();

	// Create save file with the data and save it
	UBTE_SaveGame* newsave = Cast<UBTE_SaveGame>(UGameplayStatics::CreateSaveGameObject(UBTE_SaveGame::StaticClass()));
	newsave->InitSaveGame(marketManagerSaveData, allAIPlayer_SaveData, playerSaveData, actionDatabaseSave, 
		dataManagerSaveData);
	int saveslot = savedGames.Num() + 1;
	UGameplayStatics::SaveGameToSlot(newsave, saveGameName, saveslot);
}

void ASaveGameManager::LoadGameData(FString _saveGameName, int _saveGameSlot)
{
	// Get savegame by name
	UBTE_SaveGame* loadedsavegame = Cast<UBTE_SaveGame>(UGameplayStatics::CreateSaveGameObject(UBTE_SaveGame::StaticClass()));
	loadedsavegame = Cast<UBTE_SaveGame>(UGameplayStatics::LoadGameFromSlot(_saveGameName, _saveGameSlot));

	// Initialise systems
	if (IsValid(loadedsavegame))
	{
		marketManagerSaveData = loadedsavegame->GetMarketManagerSaveData();
		allAIPlayer_SaveData = loadedsavegame->GetAllAIPlayerSaveData();
		playerSaveData = loadedsavegame->GetPlayerSaveData();
		actionDatabaseSave = loadedsavegame->GetActionDatabaseSaveData();
		dataManagerSaveData = loadedsavegame->GetDataManagerSaveData();
	}
}
