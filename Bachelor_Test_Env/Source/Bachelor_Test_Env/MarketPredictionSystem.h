// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionDatabase.h"
#include "EnumLibrary.h"
#include "CoreTypes.h"
#include "DebugDataManager.h"
#include "GameFramework/Actor.h"
#include "MarketPredictionSystem.generated.h"


// Container for the Calc Operations of the batchnormbackprop -> Used for Functions Params
USTRUCT()
struct FBBContainer
{
	GENERATED_BODY()
	FBBContainer() {}
	FBBContainer(TMap<int, TArray<float>> _bbDer)
	{
		bbDer = _bbDer;
	}

private:
	TMap<int, TArray<float>> bbDer;

public:
	TMap<int, TArray<float>> GetBBDer() { return bbDer; }

	FORCEINLINE
		void AddBBD(TArray<float> _ders) { bbDer.Add(bbDer.Num(), _ders); }
};

USTRUCT()
struct FfloatArray
{
	GENERATED_BODY()

	FfloatArray() {}

	FfloatArray(TArray<float> _array)
	{
		array = _array;
	}

private:
	UPROPERTY()
	TArray<float> array;

public:
	FORCEINLINE
		TArray<float> GetArray() { return array; }
	FORCEINLINE
		void RemoveAtPos(int _id) { array.RemoveAt(_id); }

	FORCEINLINE
		void AddAtPos(int _id, float _value) { array[_id] = _value; }
	FORCEINLINE
		void AddToArray(float _value) { array.Add(_value); }
};


USTRUCT(BlueprintType)
struct FNNNeuron
{
	GENERATED_BODY()

	FNNNeuron(int _size)
	{
		values.Init({ float() }, _size);
	}

	FNNNeuron() {}

	// Mein layer hällt eine array aus neuronen welche zu ihrer position korrospondieren, die einzelnen neuronen halten nur den container für ihre individuellen values
	// i.e values by batch
	// _values[0] == minibatch[0] usw.
	FNNNeuron(TArray<float> _values)
	{
		values = _values;
	}

private:
	// Der Key korrospondiert zu dem feature mit welcherm dieser wert assoziirt ist
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess));
	TArray<float> values;

public:
	// Value sollte eign der output des neuron sein (inc. weight, bias, activation)
	FORCEINLINE
		TArray<float> GetValues() { return values; }
	FORCEINLINE
		void AddValue(float _newEntry) { values.Add(_newEntry); }

	FORCEINLINE
		void UpdateValue(int _idx, float _newEntry) { values[_idx] = _newEntry; }
};



// Array struct for the neurons
USTRUCT(BlueprintType)
struct FNNNeuronArray
{
	GENERATED_BODY()

	FNNNeuronArray() {};

	FNNNeuronArray(TArray<FNNNeuron> _neuronArray)
	{
		neuronArray = _neuronArray;
	}

private:
	UPROPERTY()
	TArray<FNNNeuron> neuronArray;

public:
	FORCEINLINE
		TArray<FNNNeuron> GetNeuronArray() { return neuronArray; }
	FORCEINLINE
		void AddNeuronAt(int _idx, FNNNeuron _neuron) { neuronArray[_idx] = _neuron; }
	FORCEINLINE
		void AddNeuron(FNNNeuron _neuron) { neuronArray.Add(_neuron); }
};

// Das hier war mein ich layer specific
USTRUCT(BlueprintType)
struct FWeightValues
{
	GENERATED_BODY()

	FWeightValues() {}

	FWeightValues(TArray<float> _weights, TArray<float> _weightGradiants)
	{
		weights = _weights;
		weightGradiants = _weightGradiants;
	}

private:
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TArray<float> weights;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TArray<float> weightGradiants;

public:
	FORCEINLINE
		TArray<float> GetWeights() { return weights; }
	FORCEINLINE
		TArray<float> GetWeightGradiants() { return weightGradiants; }

	FORCEINLINE
		void SetWeightGradiants(TArray<float> _gradiants) { weightGradiants = _gradiants; }
	FORCEINLINE
		void SetNewWeights(TArray<float> _newWeights) { weights = _newWeights; }
};

// Das hier war mein ich nn specific
USTRUCT(BlueprintType)
struct FHHWeights
{
	GENERATED_BODY()

	FHHWeights() {}

	FHHWeights(TArray<FWeightValues> _weightValues)
	{
		weightValues = _weightValues;
	}

private:
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TArray<FWeightValues> weightValues;

public:
	FORCEINLINE
		TArray<FWeightValues> GetWeightValues() { return weightValues; }
	FORCEINLINE
		void SetGradiantByIndex(int _idx, TArray<float> _gradiants) { weightValues[_idx].SetWeightGradiants(_gradiants); }

	FORCEINLINE
		void SetWeightValues(TArray<FWeightValues> _weightVals) { weightValues = _weightVals; }
};

USTRUCT(BlueprintType)
struct FHHBiases
{
	GENERATED_BODY()

	FHHBiases() {}

	FHHBiases(TArray<float> _biases, TArray<float> _biasGradiants)
	{
		biases = _biases;
		biasGradiants = _biasGradiants;
	}

private:
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TArray<float> biases;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TArray<float> biasGradiants;

public:
	FORCEINLINE
		TArray<float> GetBiases() { return biases; }
	FORCEINLINE
		TArray<float> GetBiasGradiants() { return biasGradiants; }
	FORCEINLINE
		void SetBiasGradiants(TArray<float> _gradiants) { biasGradiants = _gradiants; }
	FORCEINLINE
		void SetNewBiasis(TArray<float> _newBiasis) { biases = _newBiasis; }
};

// Daten pro plot welche als input genommen werden, und deren output (also der preis eines jeden plots)
// Menge in relation zur menge an erfassten werten
// Im falle der seperated data sind die output values 3 x 1 und in der combined version sind diese 1 x 3
// Ich habe auch nicht 3 x FfloatArray da ich ja alle daten dann in einem habe
USTRUCT(BlueprintType)
struct FPlotInputOutputData
{
	GENERATED_BODY()

	FPlotInputOutputData() {}

	FPlotInputOutputData(TMap<int, FfloatArray> _outputValues, TMap<int, FfloatArray> _inputs)
	{
		outputValue = _outputValues;
		inputs = _inputs;
	}

private:
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TMap<int, FfloatArray> outputValue;

	UPROPERTY()
	TMap<int, FfloatArray> inputs;

public:
	FORCEINLINE
		TMap<int, FfloatArray>  GetOutput() { return outputValue; }

	FORCEINLINE
		TMap<int, FfloatArray> GetInputValues() { return inputs; }
};

USTRUCT(BlueprintType)
struct FTableInputOutputData
{
	GENERATED_BODY()

	FTableInputOutputData() {}

	FTableInputOutputData(TMap<int, FPlotInputOutputData> _data)
	{
		inputsOutputsByPlot = _data;
	}

private:
	// Ich habe ja immer drei -> Anzahl der resourcen
	// Hierbei steht der key als plot
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TMap<int, FPlotInputOutputData> inputsOutputsByPlot;

public:
	FORCEINLINE
		void AddPlot(int plotid, FPlotInputOutputData _newPlot) { inputsOutputsByPlot.Add(plotid, _newPlot); }
	FORCEINLINE
		TMap<int, FPlotInputOutputData> GetInputOutputPlots() { return inputsOutputsByPlot; }
};

USTRUCT(BlueprintType)
struct FMiniBatchData
{
	GENERATED_BODY()

	FMiniBatchData() {}

	FMiniBatchData(TArray<FPlotInputOutputData> _batchdata)
	{
		miniBatchData = _batchdata;
	}

private:
	UPROPERTY()
	TArray<FPlotInputOutputData> miniBatchData;

public:
	FORCEINLINE
		TArray<FPlotInputOutputData> GetBatchData() { return miniBatchData; }
};

UCLASS()
class BACHELOR_TEST_ENV_API AMarketPredictionSystem : public AActor
{
	GENERATED_BODY()

public:
	AMarketPredictionSystem();

protected:
	virtual void BeginPlay() override;




public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void InitPredictionSystem(class AActionDatabase* _actionDatabase);

private:
	UPROPERTY(/*VisibleAnywhere, */BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	class AActionDatabase* actionDatabase;


	UPROPERTY(/*VisibleAnywhere,*/ BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	TMap<int, FLoadedAndCuratedTable> loadedDataTables;

	// Sollte am ehesten nicht von nem constructor abhäng
	// map key wird genutzt weil ich mehrere hidden layer so unterstützen kann ohne ein seperates struct anzulegen
	// i.e 0 == i, 1 == 1. bn, 2 == 1. h usw.
	UPROPERTY(/*VisibleAnywhere,*/ BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	TMap<int, class UNNLayer*> neuralNetwork;

	// to evaluate the trainings performance over time
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	float averageMSE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	int epochs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	int iteratedEpochs;;

	// Ich nutze Mini Batch, das heißt das batchSize == größe eines trainingsbatches ist, aka menge an plot an welchen pro epoch trainiert werden
	// number of trainings features
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	int batchSize;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	int batchAmount;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	float learningRate;

	// All relevant inputs
	// Ich setzte die erst mal (ich weiß ja meinen input count)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	int inputCount;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	int outputCount;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	int hiddenNeuronCount;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	int hiddenLayerCount;


	// weights, Input -> Hidden
	// One struct per neuron, holding a list of floats representative of the weights towards each neuron in the next layer
	UPROPERTY(/*VisibleAnywhere,*/ BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	TMap<int, FWeightValues>	w_I_H;
	// weights, Hidden -> Output
	UPROPERTY(/*VisibleAnywhere, */BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	TMap<int, FWeightValues>	w_H_O;

	// Weights form hidden to hidden layer, index == hl pos in nn und key ist gleich column an werten runter, i.e 0 == weight für value 0 usw
	UPROPERTY(/*VisibleAnywhere, */BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	TMap<int, FHHWeights> w_H_H;

	// biases, Input -> Hidden
	UPROPERTY(/*VisibleAnywhere, */BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	FHHBiases b_I_H;
	// biases, Hidden -> Output
	UPROPERTY(/*VisibleAnywhere, */BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	FHHBiases b_H_O;

	// Ich muss die nesten wenn ich noch mehr hidden layer haben will, im moment habe ich ja nur 2, sprich ich brauche auch nur EINEN weight und bias container dazwischen
	UPROPERTY(/*VisibleAnywhere,*/ BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	TMap<int, FHHBiases> b_H_H;

	UPROPERTY(/*VisibleAnywhere,*/ BlueprintReadOnly, Category = Info, meta = (AllowPrivateAccess))
	TArray<FTableInputOutputData> combinedTableDatas;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TArray< FMiniBatchData> debugMiniBatchCache;

	// Indices for layerIndex in the neuron calc (should be 2 and 6)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	int inputHiddenIndex;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	int hiddenOutputIndex;

	// slopeCoefficient for leaky relu 
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
	float slopeCoefficient;

private:
	// Die Inputs welche ich hier bekomme sind in diesem fall meine plots,
	// Alle Plot values außer modprice werden genutzt um herrzuleiten was den plotpreis herrgeführt hat
	// Das heißt im grunde genommen auch das ich meine TRAININGS inputs zum herrleiten meiner TRAININGS targets und meine TEST targets im selben time series plot habe
	// Reminder; Ich habe in einem der trainings projects die targets deffiniert als tuple (purple = (1, 0, 0)) usw und ich mache die annäherung an den literal als goal da
	// Ich glaub ich kann das dann einfach als einen wert, den IST wert haben und diesen gegen den pred wert setzten WEIL Ich auch nur einen output brauche ich verfickter idiot
	// Hier heißst das ja im grunde genommen alle werte eines plots (- den wert den ich suche) == der wert den ich suche

	void InitWeights();
	UFUNCTION()
	void InitBiases();
	//UFUNCTION()
	//	void InitNeuronGradiant();

	UFUNCTION()
	TArray<FMiniBatchData> CreateMiniBatch();

	UFUNCTION()
	void InitData();

	UFUNCTION()
	void InitCombinedNetwork();

	UFUNCTION()
	void ForwardPass();
	UFUNCTION()
	void Backpropagation();

	UFUNCTION()
	void SetInputsOutputs(TArray<FPlotInputOutputData> _batchData);
	UFUNCTION()
	FBatchDebugData CreateBatchDebugData(FDateTime _time);
	UFUNCTION()
	void SampleBatchDebugData(FBatchDebugData _bdd, FDateTime _time);
	UFUNCTION()
	FMiniBatchDebugData CreateMiniBatchDebugData(FBatchDebugData _bdd, FDateTime _time);

	// For Regular Layers
	UFUNCTION()
	TMap<int, FfloatArray> TransformDataByBatch(UNNLayer* _layerToTransform);
	// For Batch Norm Layers
	UFUNCTION()
	TMap<int, FfloatArray> TransformDataByNeuron(UNNLayer* _layerToTransform);

	// Batch normalization layer
	// To be called for the neuron values of every neuron between all layers
	UFUNCTION()
	class UBatchNormLayer* BatchNormalization_F(int _batchIdx, UNNLayer* _prevLayer, int _layerIndex);
	UFUNCTION()
	// batch normalization for the batch layer at back propagation
	void BatchNormalization_B(class UBatchNormLayer* _L1, class URegularLayer* _Lm1);
	UFUNCTION()
	void CalculateNeuronValues(UNNLayer* _prevLayer, int _currLayerIndex, int _hhindex);

	UFUNCTION()
	void CalculateMean(TMap<int, FfloatArray> _neuronValues, UBatchNormLayer* _batchLayer, int _inputSize);
	UFUNCTION()
	void CalculateVariant(TMap<int, FfloatArray> _neuronValues, UBatchNormLayer* _batchLayer, int _inputSize);
	UFUNCTION()
	float CalculateTotalMean(TArray<float> _layerMeans);
	UFUNCTION()
	float CalculateTotalVariant(TArray<float> _layerVariants);
	UFUNCTION()
	TArray<float> NormalizeInputs(TArray<float> _neuronValues, float _totalMean, float _totalVariant);
	UFUNCTION()
	TArray<float> ScaleAndShift(TArray<float> _normalizedvalues, UBatchNormLayer* _currbatchlayer);

	UFUNCTION()
	TArray<float> AverageDerivatives(int _featurLenght, int _batchLenght, FBBContainer _fbbCont);

	// 1.
	UFUNCTION()
	float C_Der_Beta(UNNLayer* _Lm1, int _featureId);
	UFUNCTION()
	float C_Der_Gamma(UBatchNormLayer* _L1, URegularLayer* _Lm1, int _batchId, int _featureId, TArray<float> _derBeta);
	UFUNCTION()
	float C_Der_NormVal(UBatchNormLayer* _L1, int _batchId, int _featureId);
	UFUNCTION()
	float C_Der_Variance(UBatchNormLayer* _L1, int _batchId, int _featureId, TArray<float>  _derOut);
	UFUNCTION()
	float C_Der_Mean(UBatchNormLayer* _L1, int _batchId, int _featureId, TArray<float>  _derNorm, TArray<float> _derVariance, int _batchSize, int _featureSize);
	UFUNCTION()
	float
		C_Der_Output(UBatchNormLayer* _L1, int _batchId, int _featureId, TArray<float>  _derNorm, TArray<float> _derVariance, TArray<float> _derMean, int _batchSize);

	// for output
	//UFUNCTION()
	//	float Softmax(TArray<float> _predictions);
	//UFUNCTION()
	//	float LogLoss(TArray<float> _activations, TArray<float> _targets);
	UFUNCTION()
	float LeakyRelu(float _toActivate);
	UFUNCTION()
	float LeakyReLuDerivative(float _differentiationValue);

	UFUNCTION()
	float Linear(float _toActivate);
	UFUNCTION()
	float LinearDerivative();

	// Calculate the mean squared error
	// Ich errechne den end error gradiant in einer seperaten function, wärend ich alle
	// anderen error gradiant in den übergängen mit errechne (ref. CG_Reg_BatchNorm)
	// Der MSE wird für eine ganze Mini Batch errechnet
	//UFUNCTION()
	//	void MeanSquaredError(int _batchIdx);
	//UFUNCTION()
	//	void AveragerMSE();

	//UFUNCTION()
	//	void CalculateGradiants_O();

	// Error gradiant with respect to scales, shifts and input of from the previous layer
	// Ich generalisiere das im namen mal, wenn mein curr or prev layer out oder input ist kann ich ja von dort exception
	// _wbindex == pos for weigh and bias index in the corrosponding container
	// _ssIndex == pos for scale and sice index	in the corrosponding container
	UFUNCTION()
	void CalculateGradiants(UNNLayer* _Lm1, UNNLayer* _L1, EWeightBiasType _wbType, int _currLIndex, int _hiddenLIndex = 0);

	UPROPERTY()
	int ddebuglayerindex;

	UFUNCTION()
	void CG_Weight_Bias(UNNLayer* _L1, UNNLayer* _Lm1, EWeightBiasType _wbType, int _hiddenLIndex, TMap<int, FfloatArray> _previousOutputs);

	UFUNCTION()
	void ComputePropagatedError(EWeightBiasType _wbType, UNNLayer* _Lm1, int _LmLenght);

	// derivative of the cost wrt = C
	// derivative of the neuron value = a(L)
	// Derivative of C with respect to A(L)
	// Current is L-1, Prev L1
	UFUNCTION()
	float ErrorGradiant(float _currBatchOuts_calc, float _currBatchOuts_ex, EWeightBiasType _wbType, UNNLayer* _L1, UNNLayer* _Lm1, int _L1idx);

	UFUNCTION()
	TArray<float> GetPropagatedGradiants(EWeightBiasType _wbType, UNNLayer* _L1, UNNLayer* _Lm1, int _batch, FfloatArray _calcOuts, FfloatArray _expectedOuts);

	// derivative of the current neuron value wrt
	// derivative of the activated value of curr layer output
	// Also im grunde genommen derivative zu input gegenüber output
	// Derivative of a(L) with respect to z(L)
	// i.e derivative of the activation function with previous neuron value
	UFUNCTION()
	float Der_CurrVal_ActivVal(float _l1NeuronVal, EActivationFunction _activationFunc);

	// derivative of the activated value of curr layer output wrt
	// derivative of the weight
	// a(L-1)
	// ie. activated value of the previous neuron (neuron value)
	UFUNCTION()
	float Der_ActivVal_Weight(float _prevNeuronVal);

	UFUNCTION()
	void UpdateValuesByDerivatives();
	UFUNCTION()
	void UpdateWeights();
	UFUNCTION()
	void UpdateBiasis();
	// Scale, Shift, Mean and Variance
	UFUNCTION()
	void UpdateBatchLayers();

	// Debug Data Stuff
private:
	UPROPERTY()
	FEpochDebugData epochDebugDatas;

	UPROPERTY()
	TMap <int, int > TOTALBYPACKAGE;
};