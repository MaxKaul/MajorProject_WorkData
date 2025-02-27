// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetMathLibrary.h"
#include "DebugDataManager.generated.h"

UENUM()
enum class EElapsedCategories
{
	EC_DEFAULT,
	EC_SetInputsOutputs,

	EC_BatchNormalization_0,
	EC_BatchNormalization_1,
	EC_BatchNormalization_2,

	EC_CalculateNeuronValues_0,
	EC_CalculateNeuronValues_1,
	EC_CalculateNeuronValues_2,

	EC_OutputCalculation,

	EC_MeanSquaredError,
	EC_ForwardPass,

	EC_Reg_BatchNorm,
	EC_BatchNorm_Reg,
	EC_Backpass,


	EC_MAX_ENTRY,
};

USTRUCT(BlueprintType, Blueprintable)
struct FDebugTime
{
	GENERATED_BODY()

	FDebugTime(){}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Hash)
	FString structName_Saved = FString("NONE");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Hash)
	FString productionSiteID_Saved = FString("NONE");

public:
	FORCEINLINE
		bool operator==(const  FDebugTime& _other) const
	{
		return Equals(_other);
	}

	FORCEINLINE
		bool operator!=(const  FDebugTime& _other) const
	{
		return !Equals(_other);
	}

	FORCEINLINE
		bool Equals(const  FDebugTime& _other) const
	{
		return structName_Saved == _other.structName_Saved && productionSiteID_Saved == _other.productionSiteID_Saved;
	}

	FDebugTime(int _h, int _m, int _s)
	{
		hours = _h;
		minutes = _m;
		seconds = _s;
		//elapsed = 0;

		startSecTotal = (((_h * 60) + _m) * 60) + _s;;
	}

public:
	FORCEINLINE
	void SetStart(int _h, int _m, int _s)
	{
		hours = _h;
		minutes = _m;
		seconds = _s;
	}
	FORCEINLINE
		void SetEnd(int _h, int _m, int _s)
	{
		endHour = _h;
		endMinute = _m;
		endSecond = _s;
	}

	//FORCEINLINE
	//	void SetElapsed(int _elapsed) { elapsed = _elapsed; }
	//FORCEINLINE
	//	int GetElapsed() { return elapsed; }

	FORCEINLINE
		int GetHours() { return hours; }
	FORCEINLINE
		int GetMinutes() { return minutes; }
	FORCEINLINE
		int GetSeconds() { return seconds; }

	FORCEINLINE
		int GetEndHour() { return endHour; }
	FORCEINLINE
		int GetEndMinute() { return endMinute; }
	FORCEINLINE
		int GetEndSecond() { return endSecond; }

private:
	UPROPERTY()
	int hours;
	UPROPERTY()
	int minutes;
	UPROPERTY()
	int seconds;

	UPROPERTY()
	int endHour;
	UPROPERTY()
	int endMinute;
	UPROPERTY()
	int endSecond;

	//UPROPERTY()
	//int elapsed;
	UPROPERTY()
	int startSecTotal;
};

// Individual Mini Batch data
USTRUCT(Blueprintable, BlueprintType)
struct FMiniBatchDebugData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Hash)
	FString structName_Saved = FString("NONE");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Hash)
	FString productionSiteID_Saved = FString("NONE");

public:
	FORCEINLINE
		bool operator==(const  FMiniBatchDebugData& _other) const
	{
		return Equals(_other);
	}

	FORCEINLINE
		bool operator!=(const  FMiniBatchDebugData& _other) const
	{
		return !Equals(_other);
	}

	FORCEINLINE
		bool Equals(const  FMiniBatchDebugData& _other) const
	{
		return structName_Saved == _other.structName_Saved && productionSiteID_Saved == _other.productionSiteID_Saved;
	}

	FMiniBatchDebugData()
	{
		FDateTime datetime;
		UKismetMathLibrary::GetTimeOfDay(datetime);
		
		catTotalTimePair = 
		{
			{EElapsedCategories::EC_SetInputsOutputs, FDebugTime(datetime.Now().GetHour(), datetime.Now().GetMinute(), datetime.Now().GetSecond())},
		
			{EElapsedCategories::EC_BatchNormalization_0, FDebugTime(datetime.Now().GetHour(), datetime.Now().GetMinute(), datetime.Now().GetSecond())},
			{EElapsedCategories::EC_BatchNormalization_1, FDebugTime(datetime.Now().GetHour(), datetime.Now().GetMinute(), datetime.Now().GetSecond())},
			{EElapsedCategories::EC_BatchNormalization_2, FDebugTime(datetime.Now().GetHour(), datetime.Now().GetMinute(), datetime.Now().GetSecond())},
		
			{EElapsedCategories::EC_CalculateNeuronValues_0, FDebugTime(datetime.Now().GetHour(), datetime.Now().GetMinute(), datetime.Now().GetSecond())},
			{EElapsedCategories::EC_CalculateNeuronValues_1, FDebugTime(datetime.Now().GetHour(), datetime.Now().GetMinute(), datetime.Now().GetSecond())},
			{EElapsedCategories::EC_CalculateNeuronValues_2, FDebugTime(datetime.Now().GetHour(), datetime.Now().GetMinute(), datetime.Now().GetSecond())},
		
			{EElapsedCategories::EC_OutputCalculation, FDebugTime(datetime.Now().GetHour(), datetime.Now().GetMinute(), datetime.Now().GetSecond())},
		
			{EElapsedCategories::EC_MeanSquaredError, FDebugTime(datetime.Now().GetHour(), datetime.Now().GetMinute(), datetime.Now().GetSecond())},
			//{EElapsedCategories::EC_ForwardPass, FDebugTime()},
			//{EElapsedCategories::EC_Reg_BatchNorm, FDebugTime()},
			//{EElapsedCategories::EC_BatchNorm_Reg, FDebugTime()},
			{EElapsedCategories::EC_Backpass, FDebugTime(datetime.Now().GetHour(), datetime.Now().GetMinute(), datetime.Now().GetSecond())},
		};
	}
private:
	UPROPERTY()
	TMap<EElapsedCategories, FDebugTime> catTotalTimePair;

public:
	FORCEINLINE
		int GetStartHourOfFirst() { return catTotalTimePair.begin().Value().GetHours(); }
	FORCEINLINE
		int GetEndHourOfLast() { return catTotalTimePair.FindRef(EElapsedCategories::EC_Backpass).GetEndHour(); }

	FORCEINLINE
		int GetStartMinutesOfFirst() { return catTotalTimePair.begin().Value().GetMinutes(); }
	FORCEINLINE
		int GetEndMinutesOfLast() { return catTotalTimePair.FindRef(EElapsedCategories::EC_Backpass).GetEndMinute(); }

	FORCEINLINE
		int GetStartSecondsOfFirst() { return catTotalTimePair.begin().Value().GetSeconds(); }
	FORCEINLINE
		int GetEndSecondsOfLast() { return catTotalTimePair.FindRef(EElapsedCategories::EC_Backpass).GetEndSecond(); }

	FORCEINLINE
	int GetTotal()
	{
		int total;

		int h = catTotalTimePair.FindRef(EElapsedCategories::EC_Backpass).GetEndHour();
		int m = catTotalTimePair.FindRef(EElapsedCategories::EC_Backpass).GetEndHour();
		int s = catTotalTimePair.FindRef(EElapsedCategories::EC_Backpass).GetEndHour();

		total = (((h * 60) + m) * 60) + s;

		return total;
	}

	FORCEINLINE
	void SetEndOfLast(EElapsedCategories _cat, int _eh, int _em, int _es)
	{
		catTotalTimePair.Find(_cat)->SetEnd(_eh, _em, _es);
	}

	FORCEINLINE
		TMap<EElapsedCategories, FDebugTime> GetCatTotalTimePair() { return catTotalTimePair; }
};

// One of those objects represents the entirety of one batch, and is holding the debug data of all mini batches in the batch
USTRUCT(Blueprintable, BlueprintType)
struct FBatchDebugData
{
	GENERATED_BODY()

	FBatchDebugData() {}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Hash)
	FString structName_Saved = FString("NONE");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Hash)
	FString productionSiteID_Saved = FString("NONE");

public:
	FORCEINLINE
		bool operator==(const  FBatchDebugData& _other) const
	{
		return Equals(_other);
	}

	FORCEINLINE
		bool operator!=(const  FBatchDebugData& _other) const
	{
		return !Equals(_other);
	}

	FORCEINLINE
		bool Equals(const  FBatchDebugData& _other) const
	{
		return structName_Saved == _other.structName_Saved && productionSiteID_Saved == _other.productionSiteID_Saved;
	}

	FORCEINLINE
		void SetTotalStart(TArray<FDebugTime> _time) { totalStart = _time; }
	FORCEINLINE
		void SetTotalEnd(TArray<FDebugTime> _time) { totalEnd = _time; }

	FORCEINLINE
		void AddTotalStart(FDebugTime _time) { totalStart.Add(_time); }
	FORCEINLINE
		void AddTotalEnd(FDebugTime _time) { totalEnd.Add(_time); }

	FORCEINLINE
		TArray<FDebugTime> GetTotalStart() { return totalStart ; }
	FORCEINLINE
		TArray<FDebugTime> GetTotalEnd() { return totalEnd; }

private:
	UPROPERTY()
	TArray<FMiniBatchDebugData> indexBatchPair;

	// total batch end and start
	UPROPERTY()
	TArray<FDebugTime> totalStart;
	UPROPERTY()
	TArray<FDebugTime> totalEnd;

public:
	FORCEINLINE
	void AddBatchEntry(FMiniBatchDebugData _data) { indexBatchPair.Add(_data); }

	FORCEINLINE
		TArray<FMiniBatchDebugData> GetIndexBatchPair() { return indexBatchPair; }

	UPROPERTY()
	int startedAtWorldSecond;
};

USTRUCT(BlueprintType, Blueprintable)
struct FEpochDebugData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Hash)
	FString structName_Saved = FString("NONE");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Hash)
	FString productionSiteID_Saved = FString("NONE");

public:
	FORCEINLINE
		bool operator==(const  FEpochDebugData& _other) const
	{
		return Equals(_other);
	}

	FORCEINLINE
		bool operator!=(const  FEpochDebugData& _other) const
	{
		return !Equals(_other);
	}

	FORCEINLINE
		bool Equals(const  FEpochDebugData& _other) const
	{
		return structName_Saved == _other.structName_Saved && productionSiteID_Saved == _other.productionSiteID_Saved;
	}

private:
	UPROPERTY()
	TMap<int, FBatchDebugData> epochBatchPair;

public:
	FORCEINLINE
		void AddNewEpochData(FBatchDebugData _debugData)
	{
		epochBatchPair.Add(epochBatchPair.Num(), _debugData);
	}

	FORCEINLINE
		TMap<int, FBatchDebugData> GetEpochBatchPair() { return epochBatchPair; }
};


FORCEINLINE uint32 GetTypeHash(const  FDebugTime& _this)
{
	const uint32 Hash = FCrc::MemCrc32(&_this, sizeof(FDebugTime));
	return Hash;
}
FORCEINLINE uint32 GetTypeHash(const  FMiniBatchDebugData& _this)
{
	const uint32 Hash = FCrc::MemCrc32(&_this, sizeof(FMiniBatchDebugData));
	return Hash;
}

FORCEINLINE uint32 GetTypeHash(const  FBatchDebugData& _this)
{
	const uint32 Hash = FCrc::MemCrc32(&_this, sizeof(FBatchDebugData));
	return Hash;
}
FORCEINLINE uint32 GetTypeHash(const  FEpochDebugData& _this)
{
	const uint32 Hash = FCrc::MemCrc32(&_this, sizeof(FEpochDebugData));
	return Hash;
}
