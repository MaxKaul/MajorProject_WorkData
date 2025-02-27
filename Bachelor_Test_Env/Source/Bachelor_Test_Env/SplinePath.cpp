// Fill out your copyright notice in the Description page of Project Settings.


#include "SplinePath.h"

#include "Components/SplineComponent.h"

ASplinePath::ASplinePath()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASplinePath::BeginPlay()
{
	Super::BeginPlay();
}

void ASplinePath::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASplinePath::DrawGraph(TArray<float> _posY, bool _bIsPrediction)
{
	TArray<FVector> actual;
	TArray<FVector> prediction;

	for (int i = 0; i < _posY.Num(); ++i)
	{
		if (!_bIsPrediction)
			actual.Add(FVector(GetActorLocation().Y + _posY[i], GetActorLocation().X + i, 0));
		else
			prediction.Add(FVector(GetActorLocation().Y + _posY[i], GetActorLocation().X + (i * 100), 0));
	}


	if (!_bIsPrediction)
	{
		for (int i = 0; i < actual.Num() - 1; ++i)
		{
			FVector startpos = actual[i];
			FVector endpos = actual[i + 1];

			DrawDebugLine(GetWorld(), startpos, endpos, FColor::Red, true, -1, -1, 2);
		}
	}
	else
	{
		for (int i = 0; i < prediction.Num() - 1; ++i)
		{
			FVector startpos = prediction[i];
			FVector endpos = prediction[i + 1];
			DrawDebugLine(GetWorld(), startpos, endpos, FColor::Green, true, -1, -1, 2);
			DrawDebugSphere(GetWorld(), endpos, 4, 8, FColor::Green, true, -1, -1, 3);
		}

		FVector startpos = prediction[prediction.Num() - 2];
		FVector endpos = prediction[prediction.Num() - 1];

		DrawDebugLine(GetWorld(), startpos, endpos, FColor::Green, true, -1, -1, 2);
	}
}