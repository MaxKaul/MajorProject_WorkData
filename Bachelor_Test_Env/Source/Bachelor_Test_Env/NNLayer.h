// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnumLibrary.h"
#include "MarketPredictionSystem.h"
#include "NNLayer.generated.h"




UCLASS()
class BACHELOR_TEST_ENV_API		UNNLayer : public UObject
{
	GENERATED_BODY()

public:
	UNNLayer() {}

protected:
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	ELayerType layerType;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	int layerIndex;

	UPROPERTY()
	float meanSquaredError;

	UPROPERTY()
	TArray<float> propGrads;

public:
	FORCEINLINE
		ELayerType GetLayerType() { return layerType; }
	FORCEINLINE
		int GetLayerIndex() { return layerIndex; }

	FORCEINLINE
		void SetMSE(float _gradiant) { meanSquaredError = _gradiant; }
	//FORCEINLINE
	//	void AddMSEVlaue(float _addVal) { meanSquaredError += _addVal; }
	FORCEINLINE
		float GetMSE() { return meanSquaredError; }

	UFUNCTION()
	virtual void InitLayer_Batch(ELayerType _layerType, int _layerIndex, TArray<float> _scales, TArray<float> _shifts, TArray<float> _scaleGradiants, TArray<float> _shiftGradiants) {};
	UFUNCTION()
	virtual void InitLayer_Regular(ELayerType _layerType, int _layerIndex, TArray<FNNNeuron> _neurons) {};

	FORCEINLINE
		float GetSinglePropGrad(int _neuronIdx) { return propGrads[_neuronIdx]; }
	FORCEINLINE
		TArray<float> GetAllPropGradsRads() { return propGrads; }

	FORCEINLINE
		void AddPropGrad(float _grad) { propGrads.Add(_grad); }
	FORCEINLINE
		void SetPropGradByN(TArray<float> _grads) { propGrads = _grads; }


};

UCLASS()
class URegularLayer : public UNNLayer
{
	GENERATED_BODY()


public:
	URegularLayer() {}

protected:
	// layer map, layer by layer will be trough class
	// In the output layer it will represent the actual outputs
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
	TArray<FNNNeuron> neurons;

public:
	virtual void InitLayer_Regular(ELayerType _layerType, int _layerIndex, TArray<FNNNeuron> _neurons) override;

	UFUNCTION()
	void SetNeurons(TArray<FNNNeuron> _neurons);

	FORCEINLINE
		TArray<FNNNeuron> GetNeurons() { return neurons; }

	UFUNCTION()
	void UpdateNeuron(int _featureIdx, int _idx, float _newValue);
};

UCLASS()
class UOutputLayer : public URegularLayer
{
	GENERATED_BODY()

	UOutputLayer() {}

private:
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TArray<FNNNeuron> calculatedOutputs;

public:
	UFUNCTION()
	void SetCalculatedOutput(int _featureIdx, int _outputIdx, float _calcOutput);

	FORCEINLINE
		TArray<FNNNeuron> GetCalcOutput() { return calculatedOutputs; };

	UFUNCTION()
	void ResetOutput(int _batchSize, int _outputSize);
};

UCLASS()
class UBatchNormLayer : public UNNLayer
{
	GENERATED_BODY()

public:
	UBatchNormLayer() {}

protected:
	// Should approximate to the true variation of the approximation
	// Gamma
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TArray<float> scales;
	// Should approximate to the true mean of the approximation
	// beta
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TArray<float> shifts;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TArray<float> allMeans;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TArray<float> allVariants;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TArray<float> shiftDerivatives;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TArray<float> scaleDervivatives;


public:
	virtual void InitLayer_Batch(ELayerType _layerType, int _layerIndex, TArray<float> _scales, TArray<float> _shifts, TArray<float> _scaleGradiants, TArray<float> _shiftGradiants) override;

	FORCEINLINE
		TArray<float> GetScales() { return scales; }
	FORCEINLINE
		void SetNewScales(TArray<float> _newScales) { scales = _newScales; }
	FORCEINLINE
		void SetNewShifts(TArray<float> _newShifts) { shifts = _newShifts; }
	FORCEINLINE
		TArray<float> GetScaleGradiants() { return scaleDervivatives; }
	FORCEINLINE
		TArray<float> GetShifts() { return shifts; }
	FORCEINLINE
		TArray<float> GetShiftGradiants() { return shiftDerivatives; }
	FORCEINLINE
		void SetScaleGradiants(TArray<float> _gradiants) { scaleDervivatives = _gradiants; }
	FORCEINLINE
		void SetShiftGradiants(TArray<float> _gradiants) { shiftDerivatives = _gradiants; }

	FORCEINLINE
		void SetNewOutputs(TMap<int, FfloatArray> _outputs) { outputs = _outputs; }

	FORCEINLINE
		void SetNewNormalizedValues(TMap<int, FfloatArray> _normvals) { normVals = _normvals; }


	UFUNCTION()
	TMap<int, FfloatArray> GetOutputs() { return outputs; }
	UFUNCTION()
	TMap<int, FfloatArray> GetNormalizedVals() { return normVals; }

	FORCEINLINE
		void AddMeanToAll(int _idx, float _toAdd) { allMeans[_idx] = _toAdd; }
	FORCEINLINE
		void AddVariantToAll(int _idx, float _toAdd) { allVariants[_idx] = (_toAdd); }

	UFUNCTION()
	void InitMeans(int _lenght);
	UFUNCTION()
	void InitVariants(int _lenght);

	UFUNCTION()
	void UpdateShiftDers(TArray<float> _shiftDers);
	UFUNCTION()
	void UpdateScaleDers(TArray<float> _shiftDers);

	FORCEINLINE
		TArray<float> GetMeans() { return allMeans; }
	FORCEINLINE
		TArray<float> GetVariants() { return allVariants; }


	// Normalized, shifted and scaled values of the batch norm
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TMap<int, FfloatArray> outputs;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TMap<int, FfloatArray> normVals;
};