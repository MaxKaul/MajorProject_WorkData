// Fill out your copyright notice in the Description page of Project Settings.


#include "NNLayer.h"

#include "AIPlayer.h"


void URegularLayer::InitLayer_Regular(ELayerType _layerType, int _layerIndex, TArray<FNNNeuron> _neurons)
{
	neurons = _neurons;
	layerType = _layerType;

}

void URegularLayer::SetNeurons(TArray<FNNNeuron> _neurons)
{
	neurons = _neurons;
}

void URegularLayer::UpdateNeuron(int _featureIdx, int _neuroIdx, float _newValue)
{
	// Ich bekomme meine values hier nach batch und muss die dann  wieder auf neuron unterteilen
	// i.e example; 287 inputs sind von einer batch und ich muss
	// Ich habe 32 batches
	// 32 x 287 kommen hier insgessamt an
	// 287 x 32 soll erstellt werden

	// Es kommt immer ein input value an, also kann ich einfach input.num x batch erstellen
	neurons[_neuroIdx].UpdateValue(_featureIdx, _newValue);
}

void UOutputLayer::ResetOutput(int _batchSize, int _outputSize)
{
	calculatedOutputs.Empty();

	for (size_t i = 0; i < _outputSize; i++)
	{
		calculatedOutputs.Add(FNNNeuron(_batchSize));
	}
}

void UOutputLayer::SetCalculatedOutput(int _featureIdx, int _neuroIdx, float _newValue)
{
	// sollte eign keine probleme geben, ich ini die nicht und setzte die auch immer nur einmal

	calculatedOutputs[_neuroIdx].UpdateValue(_featureIdx, _newValue);
}


// shift = beta
// scale = gamma
void UBatchNormLayer::InitLayer_Batch(ELayerType _layerType, int _layerIndex, TArray<float> _scales, TArray<float> _shifts, TArray<float> _scaleGradiants, TArray<float> _shiftGradiants)
{
	scales = _scales;
	shifts = _shifts;
	layerType = _layerType;
	layerIndex = _layerIndex;

	scaleDervivatives = _scaleGradiants;
	shiftDerivatives = _shiftGradiants;
}

void UBatchNormLayer::InitMeans(int _lenght)
{
	allMeans.Empty();
	for (size_t i = 0; i < _lenght; i++)
	{
		allMeans.Add(0);
	}
}

void UBatchNormLayer::InitVariants(int _lenght)
{
	allVariants.Empty();
	for (size_t i = 0; i < _lenght; i++)
	{
		allVariants.Add(0);
	}
}

void UBatchNormLayer::UpdateShiftDers(TArray<float> _shiftDers)
{
	for (size_t i = 0; i < shiftDerivatives.Num(); i++)
	{
		shiftDerivatives[i] = _shiftDers[i];
	}
}

void UBatchNormLayer::UpdateScaleDers(TArray<float> _scaleDers)
{
	for (size_t i = 0; i < scaleDervivatives.Num(); i++)
	{
		scaleDervivatives[i] = _scaleDers[i];
	}
}