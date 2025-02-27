// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnumLibrary.h"
#include "Math/Vector2D.h" 
#include "Engine/DataTable.h"
#include "AttributeLibrary.generated.h"


USTRUCT(BlueprintType)
struct FAITraderAttributeInfo
{
	GENERATED_BODY()

	FAITraderAttributeInfo(){}

	// manual constructor for data reconstruction from saved data
	FAITraderAttributeInfo(EResourceIdent _prefSellIdent, int _prefSellValue, EResourceIdent _prefBuyIdent, int _prefBuyValue, EAIActions _prefStateIdent, float prefSellValue,
		int _amountMod, float _equilibriumAmount, int _resourceGoalAmount, int _currencyGoalAmount)
	{
		prefSellResource.Add(_prefSellIdent, _prefSellValue);
		prefBuyResource.Add(_prefBuyIdent, _prefBuyValue);
		prefState.Add(_prefStateIdent, prefSellValue);
		amountModifier = _amountMod;
		equilibriumAmount = _equilibriumAmount;
		resourceGoalAmount = _resourceGoalAmount;
		currencyGoalAmount = _currencyGoalAmount;
	}

private:
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
		FName prefferenceName;

	// Modifier will be applied as percentage, the value is the weight not the goal
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
		TMap<EResourceIdent, int> prefSellResource;

	// Modifier will be applied as percentage, the value is the weight not the goal
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
	TMap<EResourceIdent, int> prefBuyResource;

	// Modifier will be applied as rnd addative
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
	TMap<EAIActions, float> prefState;

	// Modifier will be applied as percentage
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
		int amountModifier;

	// Used to scale with the goal prefs aso to not set them to random lows (To be multiplied)
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess, ClampMin = 0.f, ClampMax = .5f))
		float equilibriumAmount;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
		int resourceGoalAmount;
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
		int currencyGoalAmount;

public:
	// Modifier will be applied as percentage
	FORCEINLINE
		TMap<EResourceIdent, int> GetPrefSellResource() { return prefSellResource; }
	// Modifier will be applied as percentage
	FORCEINLINE
		TMap<EResourceIdent, int> GetPrefBuyResource() { return prefBuyResource; }
	// Modifier will be applied as rnd addative
	FORCEINLINE
		TMap<EAIActions, float> GetPrefState() { return prefState; }
	// Modifier will be applied as percentage
	FORCEINLINE
		int GetAmountModifier() { return amountModifier; }
	FORCEINLINE
		int GetResourceGoalAmount() { return resourceGoalAmount; }

	FORCEINLINE
		int GetCurrencyGoalAmount() { return currencyGoalAmount; }
	FORCEINLINE
		float GetEquilibriumAmount() { return equilibriumAmount; }
};

USTRUCT(BlueprintType)
struct FAITraderAttribute : public FTableRowBase
{
	GENERATED_BODY()

		FAITraderAttribute() {}

private:
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
		TArray<FAITraderAttributeInfo> traderAttributes;

public:
	FORCEINLINE
		TArray<FAITraderAttributeInfo> GetTraderAttributes() { return traderAttributes; }
};