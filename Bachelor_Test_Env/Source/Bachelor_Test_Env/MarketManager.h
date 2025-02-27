// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnumLibrary.h"
#include "GameFramework/Actor.h"
#include "MarketManager.generated.h"


// Market Pool Info to Resources, ident will be defind by the data structure abd not this struct
// -> The Resources dont need dont need to  be defined seperatly because they cant be defined by their ident for they have no effect
USTRUCT(BlueprintType, Blueprintable)
struct FResourceMarketPoolInfo
{
	GENERATED_BODY()

	FResourceMarketPoolInfo() {}

	FResourceMarketPoolInfo(float _unModprice, float _modprice, int _amount, float _demandDelta, float _demandAlpha, 
							float _demandLambda, float _timeSinceUpdate, EResourceIdent _ident)
	{
		resourceAmount = _amount;
		unModResourcePrice = _unModprice;
		modResourcePrice = _modprice;

		demandDelta = _demandDelta;
		demandAlpha = _demandAlpha;
		demandLambda = _demandLambda;

		timeSinceUpdate = _timeSinceUpdate;

		ident = _ident;
	}


	friend class AMarketManager;

private:
	// To be adj by buy and sell actions
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
		float unModResourcePrice;
	// Modified resource price, to be used as buy and sell value
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
		float modResourcePrice;
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
		int resourceAmount;



	// Addative values representing the demand for a resource
	// While alpha and lamba are static value per resource (Only to be set once over the BP), delta is a dynamic value adding a layer based on a linear resource increase/decrease
	// The dynamic in supply and demand will come from the decay term
	// aka. DecayTerm
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
		float demandDelta;
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
		float demandAlpha;
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
		float demandLambda;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
		EResourceIdent ident;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
		float timeSinceUpdate;

public:
	// To be adj by buy and sell actions
	FORCEINLINE
		float GetUnmodifiedPrice() { return  unModResourcePrice; }
	// Modified resource price, to be used as buy and sell value
	FORCEINLINE
		float GetModifiedPrice() { return  modResourcePrice; }
	FORCEINLINE
		int GetAmount() { return  resourceAmount; }

	FORCEINLINE
		float GetDemandLambda() { return demandLambda; }
	FORCEINLINE
		float GetDemandDelta() { return demandDelta; }
	FORCEINLINE
		float GetDemandAlpha() { return demandAlpha; }
	FORCEINLINE
		float GetTimeSinceUpdate() { return timeSinceUpdate; }
	FORCEINLINE
		EResourceIdent GetIdent() { return ident; }

private:
	// Delta is createt by scaleing the base (I should find out what base value delta needed again or even if, at the end of the day a scale by 1+...)
	FORCEINLINE
		void UpdateDelta(float _scalingFactor) { demandDelta *= _scalingFactor; }

	FORCEINLINE
		void SetNewAmount(int _newAmount) { resourceAmount = _newAmount; }
	FORCEINLINE
		void SetUnmodPrice(float _newPrice) { unModResourcePrice = _newPrice; };
	FORCEINLINE
		void SetModPrice(float _newPrice) { modResourcePrice = _newPrice; };

	FORCEINLINE
		void SetLastUpdated(float _newTime) { timeSinceUpdate = _newTime; }
};

USTRUCT(BlueprintType)
struct FResourceTransactionTicket
{
	GENERATED_BODY()

		FResourceTransactionTicket(){}

	FResourceTransactionTicket(EResourceIdent _ident, int _resourceAmount, float _exchangeCurrency)
	{
		s_ident = _ident;
		s_resourceAmount = _resourceAmount;
		s_exchangeCurrency = _exchangeCurrency;
	}

private:
	UPROPERTY()
	EResourceIdent s_ident;
	UPROPERTY()
		int s_resourceAmount;
	UPROPERTY()
		float s_exchangeCurrency;

public:
	FORCEINLINE
		EResourceIdent S_GetTicketIdent() { return s_ident; }
	FORCEINLINE
		int S_GetTicketAmount() { return s_resourceAmount; }
	FORCEINLINE
		float S_GetExchangeCurrency() { return s_exchangeCurrency; }
};

USTRUCT()
struct FMarketManagerSaveData
{
	GENERATED_BODY()

		FMarketManagerSaveData() {}

	FMarketManagerSaveData(TMap<EResourceIdent, FResourceMarketPoolInfo> _currPoolInfo)
	{
		currPoolInfo = _currPoolInfo;

		if(currPoolInfo.Num() > 0 )
			bSaveWasInit = true;
	}

private:
	UPROPERTY()
		TMap<EResourceIdent, FResourceMarketPoolInfo> currPoolInfo;
	UPROPERTY()
		bool bSaveWasInit;

public:
	FORCEINLINE
		TMap<EResourceIdent, FResourceMarketPoolInfo> S_GetSavedPoolInfo() { return currPoolInfo; }

	FORCEINLINE
		bool WasSaveInit() { return bSaveWasInit; }
};


UCLASS()
class BACHELOR_TEST_ENV_API AMarketManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AMarketManager();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
		void InitMarketManager(FMarketManagerSaveData _saveGameData, class ADataManager* _dataManager, class AActionDatabase* _actionDatabase, class ASaveGameManager* _saveGameManager);
	UFUNCTION()
		int GetModResourcePrice(EResourceIdent _askedResource);

	
	UFUNCTION(BlueprintCallable)
		TArray<FResourceTransactionTicket> BuyResources(TArray<FResourceTransactionTicket> _transactionTickets);
	UFUNCTION(BlueprintCallable)
		TArray<FResourceTransactionTicket> SellResources(TArray<FResourceTransactionTicket> _transactionTickets);

	UFUNCTION()
		void UpdateResourcePrice(EResourceIdent _resourceToUpdate, bool _sell);

	UFUNCTION()
		FMarketManagerSaveData GetMarketManagerSaveData();

	FORCEINLINE
		TMap<EResourceIdent, FResourceMarketPoolInfo> GetResourcePoolInfo() { return resourcePool; }

private:
	UPROPERTY(VisibleAnywhere, Category = PoolInfo, meta=(AllowPrivateAccess));
	TMap<EResourceIdent, FResourceMarketPoolInfo> resourcePool;

	UPROPERTY(EditAnywhere, Category = PoolInfo, meta = (AllowPrivateAccess))
		FVector2D resAmMinMax;
	UPROPERTY(EditAnywhere,Category= PoolInfo, meta=(AllowPrivateAccess))
		FVector2D resPrMinMax;
	UPROPERTY(VisibleAnywhere, Category = PoolInfo, meta = (AllowPrivateAccess))
		class ADataManager* dataManager;
	UPROPERTY(VisibleAnywhere, Category = PoolInfo, meta = (AllowPrivateAccess))
	class AActionDatabase* actionDatabase;
	UPROPERTY(VisibleAnywhere, Category = PoolInfo, meta = (AllowPrivateAccess))
		ASaveGameManager* saveGameManager;

	UPROPERTY(VisibleAnywhere, Category = PoolInfo, meta = (AllowPrivateAccess))
		float currentPlotWeight;

private:
	UFUNCTION()
		void InitResources(TMap<EResourceIdent, FResourceMarketPoolInfo> _poolInfo);

	UFUNCTION()
		void TickResourcePrice();
};