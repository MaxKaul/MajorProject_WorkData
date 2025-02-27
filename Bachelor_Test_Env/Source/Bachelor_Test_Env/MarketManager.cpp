// Fill out your copyright notice in the Description page of Project Settings.


#include "MarketManager.h"

#include "ActionDatabase.h"
#include "DataManager.h"
#include "SaveGameManager.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
AMarketManager::AMarketManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMarketManager::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AMarketManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMarketManager::InitMarketManager(FMarketManagerSaveData _saveGameData, ADataManager* _dataManager, AActionDatabase* _actionDatabase, ASaveGameManager* _saveGameManager)
{
	TMap<EResourceIdent, FResourceMarketPoolInfo> savedpoolinfo;

	if (!_saveGameData.WasSaveInit())
	{
		float rndprice_0 = FMath::RandRange(resPrMinMax[0], resPrMinMax[1]);
		float rndprice_1= FMath::RandRange(resPrMinMax[0], resPrMinMax[1]);
		float rndprice_2 = FMath::RandRange(resPrMinMax[0], resPrMinMax[1]);

		savedpoolinfo =
		{
			// I CULL FLOATS, SO NO MORE THAN 4 DECIMALS

			// TODO; Ich brauche default lambda und alpha pro resource -> rnd?
			// Ich mache die wohl rnd aus einem set, sprich alpha[x] / lambda[x], aber ich hardcode die jetzt erstmal zu 2 zu 1
			// Lambda == time weight
			// Delta == inflation weight
			// Alpha == price weight
			//{EResourceIdent::RI_Stone, FResourceMarketPoolInfo( FMath::RandRange(resPrMinMax[0], resPrMinMax[1]),0, FMath::RandRange(resAmMinMax[0], resAmMinMax[1]),0.15f, 0.0055f, 0.15,0)},
			//{EResourceIdent::RI_Water, FResourceMarketPoolInfo(FMath::RandRange(resPrMinMax[0], resPrMinMax[1]),0, FMath::RandRange(resAmMinMax[0], resAmMinMax[1]), 0.15f, 0.0055f, 0.15,0)},
			//{EResourceIdent::RI_Wood,  FResourceMarketPoolInfo(FMath::RandRange(resPrMinMax[0], resPrMinMax[1]),0, FMath::RandRange(resAmMinMax[0], resAmMinMax[1]), 0.15f, 0.0055f, 0.15,0)},


			{EResourceIdent::RI_Stone, FResourceMarketPoolInfo(rndprice_0,rndprice_0, 1000 , 0.0015f, 0.0055f, 0.0015,0, EResourceIdent::RI_Stone)},
			{EResourceIdent::RI_Water, FResourceMarketPoolInfo(rndprice_1,rndprice_1, 1000 , 0.0015f, 0.0055f, 0.0015,0, EResourceIdent::RI_Water)},
			{EResourceIdent::RI_Wood,  FResourceMarketPoolInfo(rndprice_2,rndprice_2, 1000 , 0.0015f, 0.0055f, 0.0015,0,EResourceIdent::RI_Wood)},
		};
	}
	else 
		savedpoolinfo = _saveGameData.S_GetSavedPoolInfo();

	dataManager = _dataManager;
	actionDatabase = _actionDatabase;
	saveGameManager = _saveGameManager;

	InitResources(savedpoolinfo);

}

int AMarketManager::GetModResourcePrice(EResourceIdent _askedResource)
{
	return resourcePool.FindRef(_askedResource).GetModifiedPrice();
}


TArray<FResourceTransactionTicket> AMarketManager::BuyResources(TArray<FResourceTransactionTicket> _transactionTickets)
{
	// Returns resources and/or money after by action for each ticket
	TArray<FResourceTransactionTicket> returnTickets;

	// Loops over all requested buy tickets
	for(FResourceTransactionTicket ticket : _transactionTickets)
	{
		// Sets buy informations
		int returnamount = ticket.S_GetTicketAmount();
		float returncurrency = ticket.S_GetExchangeCurrency();
		EResourceIdent ident = ticket.S_GetTicketIdent();

		// Checks if the resource has been initialised in the resource pool
		if(!resourcePool.Contains(ident))
			continue;

		// Gets the resource infos by ident from the resource pool
		FResourceMarketPoolInfo poolinfo = resourcePool.FindRef(ticket.S_GetTicketIdent());

		// 1. If the buy request is higher than the marked amount, sets market amount as buy amount
		// 2. If the buy request is more expensive than the ticket currency it will be adjusted to the
		// max. amount of buyable resources
		if (poolinfo.GetAmount() - ticket.S_GetTicketAmount() < 0)
			returnamount = poolinfo.GetAmount();
		if (poolinfo.GetModifiedPrice() * ticket.S_GetTicketAmount() > ticket.S_GetExchangeCurrency())
			returnamount = ticket.S_GetExchangeCurrency() / poolinfo.GetModifiedPrice();

		// Calculates the return money based on the evaluated resource amount
		returncurrency -= returnamount * poolinfo.GetModifiedPrice();

		// Subtracts the bought resources from the market,
		// creates and adds the return ticket with the specifications and updates the price
		resourcePool.Find(ident)->SetNewAmount(poolinfo.GetAmount() - returnamount);
		FResourceTransactionTicket newticket = { ticket.S_GetTicketIdent(), returnamount, returncurrency };
		returnTickets.Add(newticket);
		UpdateResourcePrice(ident, false);
	}

	return returnTickets;
}

TArray<FResourceTransactionTicket> AMarketManager::SellResources(TArray<FResourceTransactionTicket> _transactionTickets)
{
	TArray<FResourceTransactionTicket> returntickets;

	for(FResourceTransactionTicket ticket : _transactionTickets)
	{
		EResourceIdent ident = ticket.S_GetTicketIdent();

		if (!resourcePool.Contains(ident))
			continue;

		FResourceMarketPoolInfo poolinfo = resourcePool.FindRef(ticket.S_GetTicketIdent());

		float returncurrency = poolinfo.GetModifiedPrice() * ticket.S_GetTicketAmount();

		resourcePool.Find(ident)->SetNewAmount(poolinfo.GetAmount() + ticket.S_GetTicketAmount());

		FResourceTransactionTicket newticket = { ticket.S_GetTicketIdent(),0,returncurrency };

		returntickets.Add(newticket);

		UpdateResourcePrice(ident, true);
	}

	return returntickets;
}

void AMarketManager::UpdateResourcePrice(EResourceIdent _resourceToUpdate, bool _sell)
{
	// -> Resource price at the time of update
	float lastprice = resourcePool.FindRef(_resourceToUpdate).GetUnmodifiedPrice();
	// -> Static additiv term for the price 
	float demandalpha = resourcePool.FindRef(_resourceToUpdate).GetDemandAlpha();
	// -> Static multiplication term for the amount 
	float demanddelta = resourcePool.FindRef(_resourceToUpdate).GetDemandDelta();
	// -> Dynamic additive for linear resource increase/decrease
	float demandlambda = resourcePool.FindRef(_resourceToUpdate).GetDemandLambda();
	// -> Time since last resource update
	float timeframe = resourcePool.FindRef(_resourceToUpdate).GetTimeSinceUpdate();
	// -> Current amount of resources on the market
	int resourcenamount = resourcePool.FindRef(_resourceToUpdate).GetAmount();

	if (resourcenamount <= 0)
	{
		UE_LOG(LogTemp,Warning,TEXT("AMarketManager, resourcenamount <= 0"))
		return;
	}

	float newprice = lastprice;
	float decayterm = 0;

	// Calculation of the new price in accordance to buy or sell action
	if (_sell)
	{
		decayterm = FMath::Exp(demandlambda * timeframe);
		newprice = lastprice + demandalpha * (demanddelta - resourcenamount) * decayterm;

		if (newprice <= .1f)
			newprice = .1f;
	}
	else
	{
		decayterm = FMath::Exp(-demandlambda * timeframe);
		newprice = lastprice - demandalpha * (demanddelta - resourcenamount) * decayterm;
	}

	// Modified price will be updated seperate and constantly over the forex data
	resourcePool.Find(_resourceToUpdate)->SetUnmodPrice(newprice);
	resourcePool.Find(_resourceToUpdate)->SetLastUpdated(0.f);
	resourcePool.Find(_resourceToUpdate)->UpdateDelta(decayterm);
}

FMarketManagerSaveData AMarketManager::GetMarketManagerSaveData()
{
	FMarketManagerSaveData savedata =
	{
		resourcePool,
	};

	return savedata;
}

void AMarketManager::InitResources(TMap<EResourceIdent, FResourceMarketPoolInfo> _poolInfo)
{
	for (TTuple<EResourceIdent, FResourceMarketPoolInfo> saveditem : _poolInfo)
	{
		resourcePool.Add(saveditem.Key, saveditem.Value);
	}

	FTimerHandle handle;

	GetWorld()->GetTimerManager().SetTimer(handle, this, &AMarketManager::TickResourcePrice, 1, true);

}

void AMarketManager::TickResourcePrice()
{
	FForexPlotData currdata = dataManager->GetCurrentPlotData();

	TMap<EResourceIdent, FResourceMarketPoolInfo> copypool = resourcePool;

	for(TTuple<EResourceIdent, FResourceMarketPoolInfo> resource : resourcePool)
	{
		float mod = currdata.GetClose();
		currentPlotWeight = mod;

		float unmodprice = resource.Value.GetUnmodifiedPrice();


		float normmod = UKismetMathLibrary::NormalizeToRange(mod, 0, 1);
		float modprice = unmodprice * normmod;

		float delta = resource.Value.GetDemandDelta();
		float alpha = resource.Value.GetDemandAlpha();
		float lambda = resource.Value.GetDemandLambda();


		float newupdate = resource.Value.GetTimeSinceUpdate() + 1;



		FResourceMarketPoolInfo newitem =
		{
			unmodprice,
			modprice,
			resource.Value.GetAmount(),
			delta,
			alpha,
			lambda,
			newupdate,
			resource.Key
		};

		copypool.Add(resource.Key, newitem);
	}

	resourcePool = copypool;

	TMap<int, FAITraderAttributeInfo> attributeinfos;
	TArray<FTraderInformations> traderinfos;


	for(AAIPlayer* trader : saveGameManager->GetAllAIPlayer())
	{
		traderinfos.Add(trader->GetTraderInformations());

		attributeinfos.Add(trader->GetAIPlayerIndex(), trader->GetTraderAttributeInfo());
	}

	FPlotData newplot =
	{
		resourcePool,
		attributeinfos,
		traderinfos,
		currentPlotWeight
	};

	actionDatabase->PlotMarketSituation(newplot);
}