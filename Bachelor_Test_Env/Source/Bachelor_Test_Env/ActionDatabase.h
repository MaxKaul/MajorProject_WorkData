// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIPlayer.h"
#include "AttributeLibrary.h"
#include "MarketManager.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "ActionDatabase.generated.h"


USTRUCT(BlueprintType)
struct FDataLineRange
{
	GENERATED_BODY()
		FDataLineRange(){}

private:
	UPROPERTY(EditAnywhere, meta=(AllowPrivateAccess))
	int minValue;
	UPROPERTY(EditAnywhere, meta=(AllowPrivateAccess))
	int maxValue;

public:
	FORCEINLINE
	int GetMax() { return maxValue; }
	FORCEINLINE
	int GetMin() { return minValue; }
};

// Ruleset for data creation and read out
USTRUCT(BlueprintType)
struct FDataRuleSet
{
	GENERATED_BODY()

		FDataRuleSet(){}

private:
	// startIdx == header lines, i.e the amount of lines of the data that leads ad titles
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
		int startIdx;
	// Data set line seperation by ident, corrosponds to the individual rows of the plot
	// Used to set cat by plotid (rows by category)
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
		TMap<EDataCategory, FDataLineRange> catSetByID;
	// Data set start pos to iterate trough enum on Name Value Pairing, corrosponds to the enum position
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
		TMap<EDataCategory, int> lineSepByEnum;

public:
	FORCEINLINE
		int GetStartIdx() { return startIdx; }
	FORCEINLINE
		TMap<EDataCategory, FDataLineRange > GetCatSetById() { return catSetByID; }
	FORCEINLINE
		TMap<EDataCategory, int > GetLineSepByEnum() { return lineSepByEnum; }
};


// Nach Forex D1
// Curated data will not include dat and time
USTRUCT(Blueprintable, BlueprintType)
struct FForexPlotData  : public FTableRowBase
{
	GENERATED_BODY()

	FForexPlotData(){}

	// Spread is not included in the data
	FForexPlotData(int _idx, float _open, float _high, float _low, float _close, float _volume)
	{
		idx = _idx;
		open = _open;
		high = _high;
		low = _low;
		close = _close;
		volume = _volume;
	}

private:
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	int idx;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	float open;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	float high;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	float low;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	float close;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	float volume;

public:
	FORCEINLINE
		int GetIdx() { return idx; }
	FORCEINLINE
		float GetOpen() { return open; }
	FORCEINLINE
		float GetHigh() { return high; }
	FORCEINLINE
		float GetLow() { return low; }
	FORCEINLINE
		float GetClose() { return close; }
	FORCEINLINE
		float GetVolume() { return volume; }
};


// All Plot informations are being recorded into FPlotData  																												   //
//																																											   //
// Daten welche pro plot zu erfasst und gespeichert werden müssen;																											   //
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
// Index:					Plot index (0-1000)																																   //
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
// Market Resource Info:		Informations about all resoruces on the market																								   //
//	Ident:							Ident of the individual resource																										   //
//	Resource Amount:				Amount of the individual resource on the marked																							   //
//	Unmodified Price:				Unmodified price of the individual resource on the marked																				   //
//	Modified Price:					Modified price of the individual resource on the marked																					   //
//	Demand Delta:					Demand delta of the individual resource						-> Dynamic, DECAY TERM														   //
//	Demand Alpha:					Demand Alpha of the individual resource						-> Static																	   //
//	Demand Lambda:					Demand Lambda of the individual resource					-> Static																	   //
//	Time Since Updated:				Time since the resource price was last updated (dont know if it will be important since it flows into the delta calculation, but what ever)//
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
// AI Attribute Info:			Informations about all trader attribute tables																								   //
// Index: 							Index of the Individual ai trader																										   //
// Preffered Sell Resource:			Map of preffered Sell Resource with Weight value			-> DIVIDED INTO IDENT AND VALUE												   //								   
// Preffered Buy Resource:			Map of preffered Buy Resource with Weight value				-> DIVIDED INTO IDENT AND VALUE												   //								   
// Preffered State:					Map of preffered State with Weight value					-> DIVIDED INTO IDENT AND VALUE												   //						   
// Amount Modifier:					Value for the amount modifier																											   //
// Equilibrium Amount				Value for the Equilibrium amount																										   //
// Resource Goal Amount				Value of the goal resource amount for the individual ai trader																			   //
// Currency Goal Amount				Value of the goal currency amount for the individual ai trader																			   //
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
// AI Info:						Informations about all traders																												   //
// Index:							Index of the Individual ai trader																										   //
// Currency:						Current amount of Currency of the individual ai trader																					   //
// Resource Pouch:					Map of all the trader resources								-> DIVIDED INTO IDENT AND VALUE												   //									   
// Won with Resources:				Status if an individual ai trader has won with resources																				   //
// Won with Currency:				Status if an individual ai trader has won with currency																					   //
// Lost:							Status if an individual ai trader has lost																								   //
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
// Forex Data Info:					Informations about the Forex Value used as weight																						   //
// Weight:							Weight used for the modified resource price -> Currently the Close																		   //
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------//


// Order;
// Market Resource Info:
// Ident:				
// Resource Amount:	
// Unmodified Price:	
// Modified Price:		
// Demand Delta:		
// Demand Alpha:		
// Demand Lambda:		
// Time Since Updated:
//------------------------------
// AI Attribute table:
// Preffered Sell Resource: DIVIDED INTO IDENT AND VALUE
// Preffered Buy Resource:	DIVIDED INTO IDENT AND VALUE
// Preffered State:			DIVIDED INTO IDENT AND VALUE
// Amount Modifier:			
// Equilibrium Amount		
// Resource Goal Amount		
// Currency Goal Amount
//------------------------------
// AI Informations:
// Index:				
// Currency:			
// Resource Pouch:			DIVIDED INTO RESOURCE IDENTS AND RESPECTIVE AMOUNTS
// Won with Resources:	
// Won with Currency:	
// Lost:
//------------------------------
// Forex Data:
// Weight:	
USTRUCT(BlueprintType)
struct FPlotData
{
	GENERATED_BODY()

	FPlotData(){}

	FPlotData(TMap<EResourceIdent, FResourceMarketPoolInfo> _allResourceInfos,
			TMap<int, FAITraderAttributeInfo> _allTraderAttributeInfos,
			TArray<FTraderInformations> _allTraderInfos,
			float _forexValue)
	{
		resInfo = _allResourceInfos;
		allTraderAttributeInfos = _allTraderAttributeInfos;
		allTraderInfos = _allTraderInfos;
		forexValue = _forexValue;
	}

private:
	UPROPERTY(VisibleAnywhere, meta =(AllowPrivateAccess))
		TMap<EResourceIdent, FResourceMarketPoolInfo> resInfo;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
		TMap<int, FAITraderAttributeInfo> allTraderAttributeInfos;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
		TArray<FTraderInformations> allTraderInfos;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
		float forexValue;
	
public:
	FORCEINLINE
		TMap<EResourceIdent, FResourceMarketPoolInfo> GetResourceInfo() { return resInfo; }
	FORCEINLINE
		TMap<int, FAITraderAttributeInfo> GetAllTraderAttributeInfos() { return allTraderAttributeInfos; }
	FORCEINLINE
		TArray<FTraderInformations> GetAllTraderInfos() { return allTraderInfos; }
	FORCEINLINE
		float GetForexValue() { return forexValue; }
};

USTRUCT()
struct FCuratedDataRow
{
	GENERATED_BODY()

		FCuratedDataRow(){}

	FCuratedDataRow(EDataCategory _rowCategory, TMap<EDataName , float> _rowNameValuePairs)
	{
		rowCategory = _rowCategory;
		rowNameValuePairs = _rowNameValuePairs;
	}

private:
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
		EDataCategory rowCategory;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
		TMap<EDataName, float> rowNameValuePairs;

public:
	FORCEINLINE
		EDataCategory GetRowCategory() { return rowCategory; }

	FORCEINLINE
		TMap<EDataName, float> GetRowNameValuePairs() { return rowNameValuePairs; }
};

// Raw values of the loaded Data
USTRUCT()
struct FLoadedDataRow
{
	GENERATED_BODY()

	FLoadedDataRow(){}

	FLoadedDataRow(TArray<float> _rowEntries)
	{
		rowEntries = _rowEntries;
	}

private:
	UPROPERTY(VisibleAnywhere, meta=(AllowPrivateAccess))
	TArray<float> rowEntries;

public:
	FORCEINLINE
		TArray<float> GetRowEntries() { return rowEntries; }
};

USTRUCT()
struct FCuratedTable
{
	GENERATED_BODY()

		FCuratedTable() {}

	FCuratedTable(TArray<FCuratedDataRow> _curatedRows)
	{
		curatedRows = _curatedRows;
	}

private:
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
		TArray<FCuratedDataRow> curatedRows;

public:
	FORCEINLINE
		TArray<FCuratedDataRow> GetCuratedRowEntries() { return curatedRows; }
};

USTRUCT(BlueprintType)
struct FLoadedDataTable
{
	GENERATED_BODY()

	FLoadedDataTable(){}

	FLoadedDataTable(TArray<FLoadedDataRow> _tableRows)
	{
		tableRows = _tableRows;
	}

private:
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TArray<FLoadedDataRow> tableRows;

public:
	FORCEINLINE
		TArray<FLoadedDataRow> GetAllTableRows() { return tableRows; }
};

USTRUCT(BlueprintType)
struct FActionDatabaseSaveData
{
	GENERATED_BODY()

		FActionDatabaseSaveData(){}

	FActionDatabaseSaveData(TArray<FForexPlotData> _loadedForexData)
	{
		loadedForexData = _loadedForexData;

		if (_loadedForexData.Num() > 0)
			bSaveWasInit = true;
	}

private:
	UPROPERTY()
		bool bSaveWasInit;

	UPROPERTY()
		TArray<FForexPlotData> loadedForexData;

public:

	FORCEINLINE
		bool WasSaveInit() { return bSaveWasInit; }

	FORCEINLINE
		TArray<FForexPlotData> GetLoadededForexData() { return loadedForexData; }
};

// One entire curated plot
USTRUCT(Blueprintable)
struct FCuratedPlot
{
	GENERATED_BODY()
		FCuratedPlot(){}

	FCuratedPlot(TArray<float> _plotData)
	{
		plotData = _plotData;
	}

private:
	UPROPERTY()
		TArray<float> plotData;

public:
	FORCEINLINE
	TArray<float> GetPlotData() { return plotData; }
};

USTRUCT(BlueprintType)
struct FLoadedAndCuratedTable
{
	GENERATED_BODY()

	FLoadedAndCuratedTable(){}

	FLoadedAndCuratedTable(TArray<FPlotData> _plotData)
	{
		plotData = _plotData;
	}

private:
	UPROPERTY(VisibleAnywhere, meta=(AllowPrivateAccess))
		TArray<FPlotData> plotData;

public:
	FORCEINLINE
		TArray<FPlotData> GetPlotData() { return plotData; }
};

UCLASS()
class BACHELOR_TEST_ENV_API AActionDatabase : public AActor
{
	const int TABLE_PLOTAMOUNT = 1000;
	const int FOCET_INTERVAL = 100;

	GENERATED_BODY()
	
public:	
	AActionDatabase();

protected:
	virtual void BeginPlay() override;

private:
	// Loaded Forex Data (will eat performance if Visible in editor)
	UPROPERTY(VisibleAnywhere, Category=Info,meta=(AllowPrivateAccess))
	TArray<FForexPlotData> loadedData;

	UPROPERTY(EditAnywhere, Category = Info, meta = (AllowPrivateAccess))
		FString defaultSaveDir;
	UPROPERTY(EditAnywhere, Category = Info, meta = (AllowPrivateAccess))
		FString defaultFileName;

	UPROPERTY(EditAnywhere, Category=Info,meta=(AllowPrivateAccess))
		class UMarketCurve* curve;

	UPROPERTY(EditAnywhere, Category = Info, meta = (AllowPrivateAccess))
		EDataName dataToAverage;

	UPROPERTY(/*EditAnywhere, Category = Info, meta = (AllowPrivateAccess)*/)
		TArray<FPlotData> allPlottedData;

	// All loaded tabhle merged and averaged by plot on the modified value (will eat performance if Visible in editor)
	UPROPERTY(/*EditAnywhere, Category = Info, meta = (AllowPrivateAccess)*/)
		TArray<FPlotData> averagePlotData;

	// (will eat performance if Visible in editor)
	UPROPERTY(/*VisibleAnywhere, Category = Info, meta = (AllowPrivateAccess)*/)
		TArray<FLoadedDataTable> allLoadedTables;

	// (will eat performance if Visible in editor)
	UPROPERTY(/*VisibleAnywhere, Category = Info, meta = (AllowPrivateAccess)*/)
		TMap<int, FLoadedAndCuratedTable> allLoadedTables_Reconstructed;

	UPROPERTY(VisibleAnywhere, Category = Info, meta = (AllowPrivateAccess))
		float stoneAverage;
	UPROPERTY(VisibleAnywhere, Category = Info, meta = (AllowPrivateAccess))
		float woodAverage;
	UPROPERTY(VisibleAnywhere, Category = Info, meta = (AllowPrivateAccess))
		float waterAverage;
	UPROPERTY(VisibleAnywhere, Category = Info, meta = (AllowPrivateAccess))
		class ASaveGameManager* saveGameManager;

	UPROPERTY(VisibleAnywhere, Category = Info, meta = (AllowPrivateAccess))
		int currentVolume;

	UPROPERTY(EditAnywhere, Category = Info, meta = (AllowPrivateAccess))
		int maxFloatLenght;

	UPROPERTY(EditAnywhere, Category = Info, meta = (AllowPrivateAccess))
		int plotLenght;

	UPROPERTY(EditAnywhere, Category = Info, meta = (AllowPrivateAccess))
		int resourceInfoLenght;
	UPROPERTY(EditAnywhere, Category = Info, meta = (AllowPrivateAccess))
		int attributeInfoLenght;
	UPROPERTY(EditAnywhere, Category = Info, meta = (AllowPrivateAccess))
		int aiInfoLenght;

	UPROPERTY(EditAnywhere, Category = Info, meta = (AllowPrivateAccess))
		bool bGeneratePlotData;

	UPROPERTY(EditAnywhere, Category = Info, meta = (AllowPrivateAccess))
		FDataRuleSet DataRuleSet;

	UPROPERTY(EditAnywhere, Category = Info, meta = (AllowPrivateAccess))
		int tableOverflowDebug;

	UPROPERTY(VisibleAnywhere, Category = Configs, meta = (AllowPrivateAccess))
	bool bEnableExponentialSmoothing;

	UPROPERTY(EditAnywhere, Category = Configs, meta = (AllowPrivateAccess))
	float exponentialSmoothAlpha;
	UPROPERTY(EditAnywhere, Category = Configs, meta = (AllowPrivateAccess))
	int exponentialSmoothStepLenght;

	UPROPERTY(EditAnywhere, meta=(AllowPrivateAccess))
	TSubclassOf<class ASplinePath> pathClass;

private:
	// Converst a float to a culled string
	UFUNCTION()
		FString ConvertFloatToString(float _toConvert, int _lenght);

	UFUNCTION()
		FString CreateMarketInfoString(FPlotData _plotData);
	UFUNCTION()
		FString CreateAttributeInfoString(FPlotData _plotData);
	UFUNCTION()
		FString CreateAIInfoString(FPlotData _plotData);
	UFUNCTION()
		FString CreateForexInfo(FPlotData _plotData);

	UFUNCTION()
		TMap<EDataName, float> CreateNameValuePair(EDataCategory _category,TArray<float> _rowEntries);
	// Sample the first plot to get the size
	// Since all plots and sets should be equal, I can use the first one
	UFUNCTION()
		int SampleDataSizePerPlot(FCuratedTable _firstTable);
	UFUNCTION()
		FString GetSaveDesignation();

		UFUNCTION()
		TArray<float> SimpleExponentialSmoothing(TArray<float> data, float alpha);

	UFUNCTION()
		void CurateLoadedData();
	// int == plot
	// FCuratedPlot == data of curated plot to average
	UFUNCTION()
		void MapAverageData(TMap<int, FCuratedPlot> _tableDataByPlot);
	// I want to reconstruct all loaded and curated tables back into plots
	UFUNCTION()
		void ReconstructAllTables(TArray<FCuratedTable> _allToReconstruct);

	UFUNCTION()
		TMap<EResourceIdent, FResourceMarketPoolInfo> ReconResourceRow(int _plotPos, FCuratedDataRow _dataRow);
	UFUNCTION()
		TMap<int, FAITraderAttributeInfo> ReconTraderAttributeRow(int _plotPos, FCuratedDataRow _dataRow);
	UFUNCTION()
		TArray<FTraderInformations> ReconTraderInfoRow(int _plotPos, FCuratedDataRow _dataRow);

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
		void InitActionDatabase(FActionDatabaseSaveData _savedata,  ASaveGameManager* _saveGameManager, bool _bEnableExpoSmoothing);
	UFUNCTION()
		FActionDatabaseSaveData GetActionDataBaseSaveData();

	FORCEINLINE
		TArray<FForexPlotData> GetLoadedForexData() { return loadedData; }

	// Can Save the actions on a row base per _saveText, meaning that the a time series plot should always be ONE entry
	// To save the wished Data into a readable format
	UFUNCTION(BlueprintCallable)
		bool SaveDataToFile(FString _saveDir, FString _fileName, bool _bAllowOverWriting, TArray<FPlotData> _plotArrayToSave);
	// Loads new Forex Data into the Project
	UFUNCTION(BlueprintCallable)
		TArray<FForexPlotData> LoadForexDataFromFile(FString _saveDir, FString _fileName);

	// Called per Table
	UFUNCTION(BlueprintCallable)
		void LoadCSVFromFile(FString _saveDir, FString _fileName, int _tableAmount);

	UFUNCTION(BlueprintCallable)
		TArray<FPlotData> GetAllPlottedData() { return allPlottedData; }

	UFUNCTION()
		void PlotMarketSituation(FPlotData _plotData);

	FORCEINLINE
		int GetCurrentVolume() { return currentVolume; }

	// Getter used for trainingsdata;
	FORCEINLINE
		TMap<int, FLoadedAndCuratedTable> GetReconstructedTables() { return allLoadedTables_Reconstructed; }
};