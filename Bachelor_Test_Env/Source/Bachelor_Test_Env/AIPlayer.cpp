// Fill out your copyright notice in the Description page of Project Settings.


#include "AIPlayer.h"

#include "DataManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
AAIPlayer::AAIPlayer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	defaultTickValueMod = 10.f;
}

// Called when the game starts or when spawned
void AAIPlayer::BeginPlay()
{
	Super::BeginPlay();


	resourcePouch.Add(EResourceIdent::RI_Stone,10);
	resourcePouch.Add(EResourceIdent::RI_Water,10);
	resourcePouch.Add(EResourceIdent::RI_Wood, 10);
}

// Called every frame
void AAIPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAIPlayer::InitAIPlayer(int _id, FAIPlayerSaveData _saveData, AMarketManager* _marketManager, ADataManager* _dataManager)
{
	aiPlayerID = _id;
	world = GetWorld();
	dataManager = _dataManager;

	marketManager = _marketManager;

	if (_saveData.S_WasSaveInit())
	{
		InitResources(_saveData);
		InitStates(_saveData);

		traderAttribute = _saveData.S_GetTraderAttribute();

		TickState();
	}
	else
	{
		currentState = EAIState::AS_Wait;

		statePropabilityPair.Add(EAIState::AS_Wait, 0);
		statePropabilityPair.Add(EAIState::AS_TradingAction, 0);

		validTickStates.Add(EAIState::AS_Wait);
		validTickStates.Add(EAIState::AS_TradingAction);

		SelectWorkerAttribute();
		TickState();
	}
}

FAIPlayerSaveData AAIPlayer::GetAIPlayerSaveData()
{
	FAIPlayerSaveData newsave = 
	{
		resourcePouch,
		aiPlayerID,
		statePropabilityPair,
		validTickStates,
		traderAttribute
	};

	return newsave;
}

void AAIPlayer::InternalFocet()
{
	TMap<EResourceIdent, int> copypouch = resourcePouch;

	for(TTuple<EResourceIdent, int> resource : resourcePouch)
	{
		copypouch.Add(resource.Key, resource.Value + UKismetMathLibrary::NormalizeToRange(dataManager->GetCurrentPlotData().GetVolume(), 0 ,10000));
	}

	resourcePouch = copypouch;
}

FTraderInformations AAIPlayer::GetTraderInformations()
{
	FTraderInformations infos =
	{
		aiPlayerID,
		currencyAmount,
		resourcePouch,
		bHasWon_Resources,
		bHasWon_Currency,
		bHasLost
	};

	return infos;
}

void AAIPlayer::InitResources(FAIPlayerSaveData _saveData)
{
	for(TTuple<EResourceIdent, int> saveitem : _saveData.S_GetSavedPoolInfo())
	{
		resourcePouch.Add(saveitem.Key, saveitem.Value);
	}
}

void AAIPlayer::InitStates(FAIPlayerSaveData _saveData)
{
	currentState = EAIState::AS_Wait;

	statePropabilityPair = _saveData.S_GetStatePropPair();
	validTickStates = _saveData.S_GetValidTickState();
}

void AAIPlayer::TickState()
{
	if (!marketManager)
	{
		UE_LOG(LogTemp,Warning,TEXT("AAIPlayer, !marketManager"))
		return;
	}

	if (CheckWinCondition())
	{
		UE_LOG(LogTemp, Warning, TEXT("AAIPlayer, Has Won"));
		return;
	}

	CheckIfLiquid();

	if (CheckLoseCondition())
	{
		UE_LOG(LogTemp, Warning, TEXT("AAIPlayer, Has Lost"));
		bHasLost = true;
		return;
	}


	FTimerHandle handle;
	TMap<EAIState, float> copymap = statePropabilityPair;

	int idx = 0;

	for(TTuple<EAIState, float> statevalue : statePropabilityPair)
	{
		if(validTickStates.Contains(statevalue.Key))
		{
			float newvalue = FMath::RandRange(0.f, defaultTickValue );
			newvalue += statevalue.Value;

			if (statevalue.Key == EAIState::AS_Wait)
				waitAccumulator += FMath::RandRange(0.f, defaultTickValue / defaultTickValueMod);
			else
				newvalue += waitAccumulator;

			copymap.Add(statevalue.Key, newvalue);

			if (statevalue.Value >= stateThreshholdValue)
			{
				SwitchState(statevalue.Key);
				ResetState();
				break;
			}
		}

		idx++;
	}

	if (idx < statePropabilityPair.Num())
		return;

	statePropabilityPair = copymap;
	world->GetTimerManager().SetTimer(handle, this, &AAIPlayer::TickState, tickTime, false);
}

void AAIPlayer::ResetState()
{
	TMap<EAIState, float> copymap = statePropabilityPair;

	for (TTuple<EAIState, float> statevalue : statePropabilityPair)
	{
		copymap.Add(statevalue.Key, 0);
	}

	statePropabilityPair = copymap;
	currentState = EAIState::AS_Wait;
	waitAccumulator = 0;

	TickState();
}

void AAIPlayer::SwitchState(EAIState _newState)
{
	currentState = _newState;

	switch (currentState)
	{
		case EAIState::AS_TradingAction:
			CalculateResourceScore();
			break;

		case EAIState::AS_Wait:
		case EAIState::AS_DEFAULT:
		case EAIState::AS_MAX_ENTRY:
		default:
			break;
	}
}

void AAIPlayer::CalculateResourceScore()
{
	TMap<EResourceIdent, float> calculatedvalues;

	// Calculation over which reosurece should be traded with a score given to each resource
	// for each resource

	TMap<EResourceIdent, FResourceMarketPoolInfo> poolinfo = marketManager->GetResourcePoolInfo();

	EResourceIdent sellresource = EResourceIdent::RI_DEFAULT;
	EResourceIdent buyresource = EResourceIdent::RI_DEFAULT;

	for (TTuple<EResourceIdent, int> resource : resourcePouch)
	{
		// Gr -> resource goalamount
		int goalamount = 0;
		// Rr -> current resource amount
		int resourceamount = 0;
		// Ir -> inflation (decay term/delta) factor for the resource
		float inflation = 0;
		// Gw -> Weight associated with the goal resource (I think I can just take the value I associated with the preffered resource)
		float goalweight_resource = 0;

		// Iw -> Weight value associated with the Inflation of the resource (I need to come up with something for that)
		float goalweight_currency = 0;
		// relevance score of the resource
		float relevance = 0;
		// r -> the endscore for the resource
		float resourcescore = 0;

		if (traderAttribute.GetPrefBuyResource().FindRef(resource.Key))
		{
			goalamount = traderAttribute.GetResourceGoalAmount();
			goalweight_resource = traderAttribute.GetPrefBuyResource().FindRef(resource.Key);
		}
		else
		{
			goalamount = traderAttribute.GetResourceGoalAmount() * traderAttribute.GetEquilibriumAmount();
			goalweight_resource = goalamount * traderAttribute.GetEquilibriumAmount();
		}

		if (goalamount <= 0)
			goalamount = 1;

		goalweight_currency = 1.f;

		resourceamount = resource.Value;
		inflation = poolinfo.FindRef(resource.Key).GetDemandDelta();

		relevance = 1 + FMath::Abs(goalamount - resourceamount) / goalamount;
		resourcescore = goalweight_resource * relevance + goalweight_currency * inflation;

		calculatedvalues.Add(resource.Key, resourcescore);
	}

	float highest = 0;
	float lowest = 9999999;

	for (TTuple<EResourceIdent, float> resource : calculatedvalues)
	{
		if(resource.Value > highest)
		{
			highest = resource.Value;
			buyresource = resource.Key;
		}

		if(resource.Value < lowest)
		{
			lowest = resource.Value;
			sellresource = resource.Key;

			if (marketManager->GetResourcePoolInfo().FindRef(sellresource).GetAmount() <= 0)
				UE_LOG(LogTemp,Warning,TEXT("AAIPlayer, Sellresource <= 0"))
		}
	}

	if(sellresource == EResourceIdent::RI_DEFAULT)
	{
		UE_LOG(LogTemp, Warning, TEXT("sellresource == EResourceIdent::RI_DEFAULT"));
		return;
	}

	CalculateAction(sellresource, buyresource);
}

void AAIPlayer::CalculateAction(EResourceIdent _sellResource, EResourceIdent _buyResource)
{
	EAIActions chosenaction = EAIActions::AA_DEFAULT;

	if (_sellResource == EResourceIdent::RI_DEFAULT)
		chosenaction = EAIActions::AA_BuyResources;
	else
		chosenaction = EAIActions::AA_SellResources;


 	EAIActions prefaction = traderAttribute.GetPrefState().begin().Key();
	float prefstateadd = traderAttribute.GetPrefState().begin().Value();

	TMap<EAIActions, float> actionprop = 
	{
		{EAIActions::AA_BuyResources, 0},
		{EAIActions::AA_SellResources, 0}
	};

	TMap<EAIActions, float> copymap = actionprop;

	float highest = 0;

	for (size_t i = 0; i < 1000; i++)
	{
		for(TTuple<EAIActions, float> action : actionprop)
		{
			float rnd = FMath::RandRange(0, 1);
			float newvalue = action.Value + rnd;

			if(action.Key == traderAttribute.GetPrefState().begin().Key())
				newvalue += FMath::RandRange(0.f, prefstateadd);

			copymap.Add(action.Key, newvalue);

			if (newvalue > highest)
			{
				highest = newvalue;
				chosenaction = action.Key;
			}
		}

		actionprop = copymap;
	}

	switch (chosenaction)
	{
	case EAIActions::AA_BuyResources:
		Action_BuyResources(_buyResource);
			break;

	case EAIActions::AA_SellResources:
		Actor_SellResources(_sellResource, false);
			break;


	case EAIActions::AA_MAX_ENTRY:
	case EAIActions::AA_DEFAULT:
		default:
			break;
	}
}

void AAIPlayer::Action_BuyResources(EResourceIdent _resourceToBuy)
{
	int resourceinpouch = resourcePouch.FindRef(_resourceToBuy);

	float alpha = 1.f;
	int goalamount_resource = 0;

	if(traderAttribute.GetPrefBuyResource().begin().Key() == _resourceToBuy)
		goalamount_resource = traderAttribute.GetPrefBuyResource().begin().Value();
	else
		goalamount_resource = traderAttribute.GetEquilibriumAmount() * traderAttribute.GetPrefBuyResource().begin().Value();

	alpha -= UKismetMathLibrary::NormalizeToRange(marketManager->GetResourcePoolInfo().FindRef(_resourceToBuy).GetModifiedPrice(),0.f,100.f);

	// alpha higher == buy more
	// alpha lower == buy less
	int tobuy = FMath::Abs(alpha * (goalamount_resource - resourceinpouch));
	float percent = (float)tobuy / 100.f;

	tobuy = percent * traderAttribute.GetPrefBuyResource().begin().Value();
	float modprice = marketManager->GetResourcePoolInfo().FindRef(_resourceToBuy).GetModifiedPrice();

	float exchangecurrency = tobuy * modprice;

	if (currencyAmount <= 0)
		return;

	if(exchangecurrency > currencyAmount)
	{
		tobuy = modprice / currencyAmount;
		exchangecurrency = tobuy * modprice;
	}

	currencyAmount -= exchangecurrency;

	FResourceTransactionTicket ticket = 
	{
		_resourceToBuy,
		tobuy,
		exchangecurrency
	};

	TArray<FResourceTransactionTicket> returntickets =  marketManager->BuyResources(TArray<FResourceTransactionTicket>{ ticket });

	for(FResourceTransactionTicket returnticket : returntickets)
	{
		currencyAmount += returnticket.S_GetExchangeCurrency();

		int amount = resourcePouch.FindRef(_resourceToBuy);
		amount += returnticket.S_GetTicketAmount();

		resourcePouch.Add(returnticket.S_GetTicketIdent(), amount);
	}
}

void AAIPlayer::Actor_SellResources(EResourceIdent _resourceToSell, bool _bsellAll)
{
	int resourceinpouch = resourcePouch.FindRef(_resourceToSell);

	float alpha = 1.f;
	int goalamount_resource = 0;

	if (traderAttribute.GetPrefSellResource().begin().Key() == _resourceToSell)
		goalamount_resource = traderAttribute.GetPrefBuyResource().begin().Value();
	else
		goalamount_resource = traderAttribute.GetEquilibriumAmount() * traderAttribute.GetPrefBuyResource().begin().Value();

	alpha -= UKismetMathLibrary::NormalizeToRange(marketManager->GetResourcePoolInfo().FindRef(_resourceToSell).GetModifiedPrice(), 0.f, 1000.f);

	int amounttosell = FMath::Abs(alpha * (goalamount_resource - resourceinpouch));
	float percent = (float)amounttosell / 100.f;
	amounttosell = percent * traderAttribute.GetPrefSellResource().begin().Value();

	int endpouchamount = resourceinpouch - amounttosell;

	if(endpouchamount < 0 || _bsellAll)
	{
		amounttosell = resourceinpouch;
		endpouchamount = 0;
	}

	resourcePouch.Add(_resourceToSell, endpouchamount);


	FResourceTransactionTicket ticket =
	{
		_resourceToSell,
		amounttosell,
		0
	};

	TArray<FResourceTransactionTicket> returntickets = marketManager->SellResources(TArray<FResourceTransactionTicket>{ ticket });

	for (FResourceTransactionTicket returnticket : returntickets)
	{
		currencyAmount += returnticket.S_GetExchangeCurrency();

		int amount = resourcePouch.FindRef(_resourceToSell);

		amount += returnticket.S_GetTicketAmount();

		resourcePouch.Add(returnticket.S_GetTicketIdent(), amount);
	}
}

void AAIPlayer::SelectWorkerAttribute()
{
	TArray<FAITraderAttribute*> attributes;

	for (TTuple<FName, unsigned char*> rowitem : attributeDataTable->GetRowMap())
	{
		attributes.Add(reinterpret_cast<FAITraderAttribute*>(rowitem.Value));
	}

	int lenght = attributes[0]->GetTraderAttributes().Num() - 1;

	int rnd = FMath::RandRange(0, lenght);

	// First row in data table
	traderAttribute = attributes[0]->GetTraderAttributes()[aiPlayerID];
}

bool AAIPlayer::CheckWinCondition()
{
	bool bstatus = false;

	if (currencyAmount >= traderAttribute.GetCurrencyGoalAmount())
	{
		bstatus = true;
		bHasWon_Currency = true;
		UE_LOG(LogTemp,Warning,TEXT("WON"))
	}
	else if(resourcePouch.FindRef(traderAttribute.GetPrefBuyResource().begin().Key()) >= traderAttribute.GetResourceGoalAmount())
	{
		bstatus = true;
		bHasWon_Resources = true;
		UE_LOG(LogTemp,Warning,TEXT("WON"))
	}

	return bstatus;
}

bool AAIPlayer::CheckIfLiquid()
{
	bool bstatus = true;

	if (currencyAmount <= 0)
	{
		TMap<EResourceIdent, int> sellmap;

		for (TTuple<EResourceIdent, int> res : resourcePouch)
		{
			if (res.Value > 0)
				sellmap.Add(res);
		}

		if (sellmap.Num() <= 0)
			bstatus = false;
		else if(sellmap.Num() > 0)
		{
			for (TTuple<EResourceIdent, int> res : resourcePouch)
				Actor_SellResources(res.Key, false);
		}
	}

	return bstatus;
}


bool AAIPlayer::CheckLoseCondition()
{
	bool bstatus = false;

	if(currencyAmount <= 0 )
	{
		int emptyamount = 0;

		for(TTuple<EResourceIdent, int> res : resourcePouch)
		{
			if (res.Value <= 0)
				emptyamount++;
		}

		if (emptyamount >= resourcePouch.Num() - 1)
		{
			bstatus = true;
			UE_LOG(LogTemp,Warning,TEXT("LOST"))
		}
	}

	return bstatus;
}

