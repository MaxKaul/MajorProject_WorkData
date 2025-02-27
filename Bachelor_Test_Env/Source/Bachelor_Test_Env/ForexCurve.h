// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ForexCurve.generated.h"

USTRUCT(BlueprintType)
struct BACHELOR_TEST_ENV_API FRuntimeMarketCurve
{
	GENERATED_USTRUCT_BODY()
public:
	FRuntimeMarketCurve();

	UPROPERTY()
	FRichCurve resourceCurves[3];

	UPROPERTY(EditAnywhere, Category = RuntimeFloatCurve)
	TObjectPtr<class UMarketCurve> ExternalCurve;
	
	FVector GetValue(float _inTime) const;

	/** Get the current curve struct */
	FRichCurve* GetRichCurve(int32 _idx);
	const FRichCurve* GetRichCurveConst(int32 _idx) const;
};

UCLASS(BlueprintType)
class BACHELOR_TEST_ENV_API UMarketCurve : public UCurveBase
{
	GENERATED_BODY()

public:

	UMarketCurve(){};

	UMarketCurve(const FObjectInitializer& ObjectInitializer);

	// X == Stone
	UPROPERTY()
	FRichCurve floatCurves[3];

	UPROPERTY()
	bool	bIsEventCurve;


	UFUNCTION(BlueprintCallable, Category="Math|Curves")
	FVector GetVectorValue(float InTime) const;
	
	virtual TArray<FRichCurveEditInfoConst> GetCurves() const override;
	virtual TArray<FRichCurveEditInfo> GetCurves() override;
	virtual bool IsValidCurve( FRichCurveEditInfo CurveInfo ) override;

	virtual FLinearColor GetCurveColor(FRichCurveEditInfo CurveInfo) const;

	bool operator == (const UMarketCurve& Curve) const;
};
