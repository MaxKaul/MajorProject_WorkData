// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SplinePath.generated.h"

UCLASS()
class BACHELOR_TEST_ENV_API ASplinePath : public AActor
{
	GENERATED_BODY()
	
public:
	ASplinePath();

	UFUNCTION()
	void DrawGraph(TArray<float> _posY, bool _bIsPrediction);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
};
