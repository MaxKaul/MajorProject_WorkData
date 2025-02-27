// Fill out your copyright notice in the Description page of Project Settings.

#include "ForexCurve.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Curves/RichCurve.h"
#include "Curves/CurveBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ForexCurve)

FRuntimeMarketCurve::FRuntimeMarketCurve()
	: ExternalCurve(nullptr)
{
}

FRichCurve* FRuntimeMarketCurve::GetRichCurve(int32 _idx)
{
	if (_idx < 0 || _idx >= 3)
		return nullptr;

	if (ExternalCurve != nullptr)
		return &(ExternalCurve->floatCurves[_idx]);
	else
		return &resourceCurves[_idx];
}

const FRichCurve* FRuntimeMarketCurve::GetRichCurveConst(int32 _idx) const
{
	if (_idx < 0 || _idx >= 3)
		return nullptr;

	if (ExternalCurve != nullptr)
		return &(ExternalCurve->floatCurves[_idx]);
	else
		return &resourceCurves[_idx];
}

UMarketCurve::UMarketCurve(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

FVector UMarketCurve::GetVectorValue(float _inTime) const
{
	FVector result;
	result.X = floatCurves[0].Eval(_inTime);
	result.Y = floatCurves[1].Eval(_inTime);
	result.Z = floatCurves[2].Eval(_inTime);

	return result;
}

static const FName XCurveName(TEXT("Stone"));
static const FName YCurveName(TEXT("Water"));
static const FName ZCurveName(TEXT("Wood"));

TArray<FRichCurveEditInfoConst> UMarketCurve::GetCurves() const
{
	TArray<FRichCurveEditInfoConst> Curves;
	
	Curves.Add(FRichCurveEditInfoConst(&floatCurves[0], XCurveName));
	Curves.Add(FRichCurveEditInfoConst(&floatCurves[1], YCurveName));
	Curves.Add(FRichCurveEditInfoConst(&floatCurves[2], ZCurveName));
	return Curves;
}

TArray<FRichCurveEditInfo> UMarketCurve::GetCurves()
{
	TArray<FRichCurveEditInfo> Curves;
	Curves.Add(FRichCurveEditInfo(&floatCurves[0], XCurveName));
	Curves.Add(FRichCurveEditInfo(&floatCurves[1], YCurveName));
	Curves.Add(FRichCurveEditInfo(&floatCurves[2], ZCurveName));
	return Curves;
}

bool UMarketCurve::IsValidCurve(FRichCurveEditInfo CurveInfo)
{
	return CurveInfo.CurveToEdit == &floatCurves[0] ||
		CurveInfo.CurveToEdit == &floatCurves[1] ||
		CurveInfo.CurveToEdit == &floatCurves[2];
}

FLinearColor UMarketCurve::GetCurveColor(FRichCurveEditInfo CurveInfo) const
{
	if (CurveInfo.CurveName == "Stone")
		return FLinearColor(0, 50, 0);
	else if (CurveInfo.CurveName == "Water")
		return FLinearColor(150, 0, 1);
	else if (CurveInfo.CurveName == "Wood")
		return FLinearColor(0, 0, 150);

	return FLinearColor(0, 0.5f, 1);
}

bool UMarketCurve::operator==(const UMarketCurve& Curve) const
{
	return (floatCurves[0] == Curve.floatCurves[0]) && 
		   (floatCurves[1] == Curve.floatCurves[1]) && 
		   (floatCurves[2] == Curve.floatCurves[2]);
}
