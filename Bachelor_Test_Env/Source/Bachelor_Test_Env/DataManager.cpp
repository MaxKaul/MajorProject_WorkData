

#include "DataManager.h"

#include "ActionDatabase.h"
#include "Kismet/KismetSystemLibrary.h"

ADataManager::ADataManager()
{
	PrimaryActorTick.bCanEverTick = true;

}

void ADataManager::BeginPlay()
{
	Super::BeginPlay();
}

void ADataManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FDataManagerSaveData ADataManager::GetDataManagerSaveData()
{
	FDataManagerSaveData newsave =
	{
		currentPosition
	};

	return newsave;
}

void ADataManager::InitDataManager(FDataManagerSaveData _saveData, AActionDatabase* _actionDataBase)
{
	world = GetWorld();

	actionDataBase = _actionDataBase;

	loadedData = actionDataBase->GetLoadedForexData();

	currentPosition = _saveData.GetSavedPlotIdx();

	FTimerHandle handle;
	world->GetTimerManager().SetTimer(handle, this, &ADataManager::TickDataModifier, dataModifierTickRate, true, 0);
}

void ADataManager::TickDataModifier()
{
	if(loadedData.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ADataManager, loadedData.Num() <= 0"))

		loadedData = actionDataBase->GetLoadedForexData();
		return;
	}

	currentPosition++;

	if (loadedData.Num() > currentPosition)
		currentPlotData = loadedData[currentPosition];
	else
	{
		dataPlotLoopAmount++;
		currentPosition = 0;
	}
}