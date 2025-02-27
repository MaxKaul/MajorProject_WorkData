
// Fill out your copyright notice in the Description page of Project Settings.


#include "BTE_SaveGame.h"


void UBTE_SaveGame::InitSaveGame(FMarketManagerSaveData _marketManagerSaveData, TArray<FAIPlayerSaveData> _allAIPlayer_SaveData, FPlayerSaveData _playerSaveData,
								 FActionDatabaseSaveData _actionDatabaseSaveData, FDataManagerSaveData _dataManagersaveData)
{
	marketManagerSaveData = _marketManagerSaveData;
	allAIPlayerSaveData = _allAIPlayer_SaveData;
	playerSaveData = _playerSaveData;
	actionDatabaseSaveData = _actionDatabaseSaveData;
	dataManagersaveData = _dataManagersaveData;
}
