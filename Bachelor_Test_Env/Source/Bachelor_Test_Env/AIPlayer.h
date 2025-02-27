 // Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeLibrary.h"
#include "EnumLibrary.h"
#include "MarketManager.h"
#include "GameFramework/Actor.h"
#include "AIPlayer.generated.h"


USTRUCT()
struct FAIPlayerSaveData
{
	GENERATED_BODY()


	FAIPlayerSaveData(TMap<EResourceIdent, int> _resourcePouch, int _aiPlayerID, TMap<EAIState, float> _statePropabilityPair,
					  TArray<EAIState> _validTickStates, FAITraderAttributeInfo _traderAttribute)
	{
		s_resourcePouch = _resourcePouch;
		s_aiPlayerID = _aiPlayerID;
		s_statePropabilityPair = _statePropabilityPair;
		s_validTickStates = _validTickStates;

		s_bSaveWasInit = true;
	}

	FAIPlayerSaveData()
	{
		s_bSaveWasInit = false;
	}

private:
	UPROPERTY()
		TMap<EResourceIdent, int> s_resourcePouch;
	UPROPERTY()
		bool s_bSaveWasInit;
	UPROPERTY()
		int s_aiPlayerID;
	UPROPERTY()
		TMap<EAIState, float> s_statePropabilityPair;
	UPROPERTY()
		TArray<EAIState> s_validTickStates;
	UPROPERTY()
	FAITraderAttributeInfo s_traderAttribute;

public:
	FORCEINLINE
		TMap<EResourceIdent, int> S_GetSavedPoolInfo() { return s_resourcePouch; }
	FORCEINLINE
		int S_GetAiPlayerID() { return s_aiPlayerID; }

	FORCEINLINE
		bool S_WasSaveInit() { return s_bSaveWasInit; }
	FORCEINLINE
		TMap<EAIState, float> S_GetStatePropPair() { return s_statePropabilityPair; }
	FORCEINLINE
		TArray<EAIState> S_GetValidTickState() { return s_validTickStates; }
	FORCEINLINE
		FAITraderAttributeInfo S_GetTraderAttribute() { return s_traderAttribute; }
};

// Used for the plot data
 USTRUCT(BlueprintType)
 struct FTraderInformations
 {
	 GENERATED_BODY()

		 FTraderInformations(){}

	 FTraderInformations(int _idx, float _currency, TMap<EResourceIdent, int> _pouch, bool _bWonWithRes, bool _bWonWithCurr, bool _bHasLost)
	 {
		 traderIdent = _idx;
		 currency = _currency;
		 currResourcePouch = _pouch;
		 bWonWithRes = _bWonWithRes;
		 bWonWithCurr = _bWonWithCurr;
		 bHasLost = _bHasLost;
	 }

 private:
	 UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
		 int traderIdent;
	 UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
		 float currency;
	 UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
		 TMap<EResourceIdent, int> currResourcePouch;
	 UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
		 bool bWonWithRes;
	 UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
		 bool bWonWithCurr;
	 UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
		 bool bHasLost;


 public:
	FORCEINLINE
		int GetIdent() { return  traderIdent; }
	FORCEINLINE
		float GetCurrency() { return  currency; }
	FORCEINLINE
		TMap<EResourceIdent, int> GetPouch() { return  currResourcePouch; }
	FORCEINLINE
		bool GetWonWithRes() { return  bWonWithRes; }
	FORCEINLINE
		bool GetWonWithCurr() { return  bWonWithCurr; }
	FORCEINLINE
		bool GetHasLost() { return  bHasLost; }
 };

UCLASS()
class BACHELOR_TEST_ENV_API AAIPlayer : public AActor
{
	GENERATED_BODY()
	
public:	
	AAIPlayer();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;


public:	

	UFUNCTION()
		void InitAIPlayer(int _id, FAIPlayerSaveData _saveData, AMarketManager* _marketManager, class ADataManager* _dataManager);

	UFUNCTION()
		FAIPlayerSaveData GetAIPlayerSaveData();
	UFUNCTION()
		void InternalFocet();

	FORCEINLINE UFUNCTION(BlueprintCallable)
		bool GetHasWonWithCurrency() { return bHasWon_Currency; }
	FORCEINLINE UFUNCTION(BlueprintCallable)
		bool GetHasWonWithResources() { return bHasWon_Resources; }
	FORCEINLINE UFUNCTION(BlueprintCallable)
		bool GetHasLost() { return bHasLost; }
	FORCEINLINE UFUNCTION(BlueprintCallable)
		int GetAIPlayerIndex() { return aiPlayerID; }
	FORCEINLINE UFUNCTION(BlueprintCallable)
		FAITraderAttributeInfo GetTraderAttributeInfo() { return traderAttribute; }

	UFUNCTION()
		FTraderInformations GetTraderInformations();

private:
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
		int aiPlayerID;

	UPROPERTY(VisibleAnywhere, Category = ResourcePouch, meta = (AllowPrivateAccess))
		TMap<EResourceIdent, int> resourcePouch;
	UPROPERTY(EditAnywhere, Category = ResourcePouch, meta = (AllowPrivateAccess))
		float currencyAmount;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AIStates, meta = (AllowPrivateAccess))
		EAIState currentState;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AIStates, meta = (AllowPrivateAccess))
		TMap<EAIState, float> statePropabilityPair;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AIStates, meta = (AllowPrivateAccess))
		TArray<EAIState> validTickStates;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AIStates, meta = (AllowPrivateAccess))
		float defaultTickValue;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AIStates, meta = (AllowPrivateAccess))
		float defaultTickValueMod;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AIStates, meta = (AllowPrivateAccess))
		float tickTime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AIStates, meta = (AllowPrivateAccess))
		float stateThreshholdValue;;
	// This value ticks up by a rnd amount with each state update and gets added towards the trading state (to negate waiting time)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AIStates, meta = (AllowPrivateAccess))
		float waitAccumulator;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AIStates, meta = (AllowPrivateAccess))
		ADataManager* dataManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AIStates, meta = (AllowPrivateAccess))
		AMarketManager* marketManager;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
	UDataTable* attributeDataTable;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	FAITraderAttributeInfo traderAttribute;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AIStates, meta = (AllowPrivateAccess))
		bool bHasLost;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AIStates, meta = (AllowPrivateAccess))
		bool bHasWon_Resources;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AIStates, meta = (AllowPrivateAccess))
		bool bHasWon_Currency;

	UPROPERTY()
		UWorld* world;

private:
	UFUNCTION()
		void InitResources(FAIPlayerSaveData _saveData);
	UFUNCTION()
		void InitStates(FAIPlayerSaveData _saveData);
	UFUNCTION()
		void TickState();
	UFUNCTION()
		void ResetState();
	UFUNCTION()
		void SwitchState(EAIState _newState);
	UFUNCTION()
		void CalculateResourceScore();
	UFUNCTION()
		void CalculateAction(EResourceIdent _sellResource, EResourceIdent _buyResource);

	UFUNCTION()
		void Action_BuyResources(EResourceIdent _resourceToBuy);
	UFUNCTION()
		void Actor_SellResources(EResourceIdent _resourceToSell, bool _bsellAll);

	UFUNCTION()
		void SelectWorkerAttribute();

	UFUNCTION()
		bool CheckWinCondition();
	UFUNCTION()
		bool CheckIfLiquid();
	UFUNCTION()
		bool CheckLoseCondition();

};
