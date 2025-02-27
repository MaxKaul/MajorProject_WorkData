// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnumLibrary.generated.h"

UENUM(BlueprintType)
enum class EResourceIdent
{
	RI_DEFAULT,
	RI_Stone,
	RI_Water,
	RI_Wood,
	RI_MAX_ENTRY
};

UENUM(BlueprintType)
enum class EAIState
{
	AS_DEFAULT,
	AS_Wait,
	AS_TradingAction,
	AS_MAX_ENTRY
};

UENUM()
enum class EAIActions
{
	AA_DEFAULT,
	AA_BuyResources,
	AA_SellResources,
	AA_MAX_ENTRY
};

// Enum values correspond the their loaded positions within their category
// Example:
// DT_AmountModifier would be 7 in 2
UENUM(BlueprintType)
enum class EDataName
{
	DN_DEFAULT					 ,
	// DC_Index
	DT_PlotIndex				 ,
								 
	// DC_MarketResourceInfo	 
	DT_ResourceIdent 			 ,
	DT_ResourceAmount			 ,
	DT_UnmodPrice				 ,
	DT_ModPrice					 ,
	DT_DemandDelta				 ,
	DT_DemandAlpha				 ,
	DT_DemandLambda				 ,
	DT_TimeSinceUpdate			 ,
								 
	// DC_AIAttributeInfo		 
	DT_AIIndexAttribute			 ,
	DT_PrefSellResource_Ident	 ,
	DT_PrefSellResource_Value	 ,
	DT_PrefBuyResource_Ident	 ,
	DT_PrefBuyResource_Value	 ,
	DT_PrefState_Ident			 ,
	DT_PrefState_Value			 ,
	DT_AmountModifier			 ,
	DT_EquilibriumAmount		 ,
	DT_ResourceGoalAmount		 ,
	DT_CurrencyGoalAmount		 ,
								 
	// DC_AIInfo				 
	DT_AIIndex					 ,
	DT_Currency					 ,
	// Ich hardcode den shit erstmal auf 3 resourcen, muss da unter umständen noch einen workaround machen
	DT_ResourcePouch_Ident_0		 ,
	DT_ResourcePouch_Value_0		 ,
	DT_ResourcePouch_Ident_1,
	DT_ResourcePouch_Value_1,
	DT_ResourcePouch_Ident_2,
	DT_ResourcePouch_Value_2,
	DT_WonWithResources			 ,
	DT_WonWithCurrency			 ,
	DT_HasLost					 ,
								 
	// DC_ForexDataInfo			 
	DT_Weight					 ,
	DN_MAX_ENTRY,
};

// Values are representative for the entry amount of the categories
// Ich starte bei 0, wenn ich auf menge setzte brauche ich kein + 1 
UENUM(Blueprintable)
enum class EDataCategory
{
	DC_DEFAULT,
	DC_Index					 ,
	DC_MarketResourceInfo		 ,
	DC_AIAttributeInfo			 ,
	DC_AIInfo					 ,
	DC_ForexDataInfo			 ,
	DC_MAX_ENTRY,
};

UENUM()
enum class ELayerType
{
	LT_DEFAULT,
	LT_Input,
	LT_Hidden,
	LT_Output,
	LT_BatchNorm,
	LT_MAX_ENTRY
};

// Ich habe die shifts und scales in meinen batch layern
// Korrospondiert zur den weights und biasis in relation zum forward pass
UENUM()
enum class EWeightBiasType
{
	BT_DEFAULT,
	BT_Input_Hidden,
	BT_Hidden_Hidden,
	BT_Hidden_Output,
	BT_MAX_ENTRY
};

UENUM()
enum class EActivationFunction
{
	AF_DEFAULT,
	AF_Linear,
	AF_LeakyRelu,
	AF_MAX_ENTRY
};