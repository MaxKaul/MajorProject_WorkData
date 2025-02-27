// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionDatabase.h"
#include "GameFramework/Actor.h"
#include "DataManager.generated.h"


USTRUCT()
struct FDataManagerSaveData
{
	GENERATED_BODY()

		FDataManagerSaveData() {}

	FDataManagerSaveData(int _currPlotIdx)
	{
		currSavedPlot = _currPlotIdx;
	}

private:
	UPROPERTY()
		bool bSaveWasInit;
	UPROPERTY()
		int currSavedPlot;

public:
	FORCEINLINE
		bool WasSaveInit() { return bSaveWasInit; }
	FORCEINLINE
		int GetSavedPlotIdx() { return currSavedPlot; }
};


UCLASS()
class BACHELOR_TEST_ENV_API ADataManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ADataManager();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	FORCEINLINE
		FForexPlotData GetCurrentPlotData() { return currentPlotData; }

	UFUNCTION()
		FDataManagerSaveData GetDataManagerSaveData();

	UFUNCTION()
		void InitDataManager(FDataManagerSaveData _saveData, AActionDatabase* _actionDataBase);
private:
	// Basically the mod on the basis of the forex data
	UFUNCTION()
		void TickDataModifier();

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DataManagement, meta = (AllowPrivateAccess))
		class AActionDatabase* actionDataBase;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataManagement, meta = (AllowPrivateAccess))
		TArray<FForexPlotData> loadedData;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataManagement, meta = (AllowPrivateAccess))
		UWorld* world;

	// Is Project Dir
	UPROPERTY(EditAnywhere, Category = Info, meta = (AllowPrivateAccess))
		float dataModifierTickRate;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataManagement, meta = (AllowPrivateAccess))
		FForexPlotData currentPlotData;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataManagement, meta = (AllowPrivateAccess))
		int currentPosition;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataManagement, meta = (AllowPrivateAccess))
		int dataPlotLoopAmount;
};
