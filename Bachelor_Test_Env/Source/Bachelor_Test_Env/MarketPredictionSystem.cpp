// Fill out your copyright notice in the Description page of Project Settings.


#include "MarketPredictionSystem.h"

#include "ActionDatabase.h"
#include "NNLayer.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Microsoft/AllowMicrosoftPlatformTypes.h"

#include <random>

#include "NavigationSystemTypes.h"

AMarketPredictionSystem::AMarketPredictionSystem()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AMarketPredictionSystem::BeginPlay()
{
	Super::BeginPlay();
}



void AMarketPredictionSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMarketPredictionSystem::InitPredictionSystem(AActionDatabase* _actionDatabase)
{
	actionDatabase = _actionDatabase;

	loadedDataTables = actionDatabase->GetReconstructedTables();

	InitWeights();
	InitBiases();

	UE_LOG(LogTemp, Warning, TEXT("Curating data with lenght of %i... expected time: %f min"), loadedDataTables.Num(), ((float)loadedDataTables.Num() * 4.5f) / 60.f);

	FTimerHandle handle;
	GetWorld()->GetTimerManager().SetTimer(handle, this, &AMarketPredictionSystem::InitData, 1.f, false);
}

void AMarketPredictionSystem::InitWeights()
{
	// Input -> Hidden
	// Paper -> https://arxiv.org/abs/1502.01852v1
	// https://www.geeksforgeeks.org/kaiming-initialization-in-deep-learning/
	// https://machinelearningmastery.com/weight-initialization-for-deep-learning-neural-networks/

	// Kaiming HE Normal Initialisation
	// Loop for all neurons of the next layer over all neurons of the current layer
	for (size_t h = 0; h < hiddenNeuronCount; h++)
	{
		TArray<float> weights;
		TArray<float> gradiants_i_h;

		for (size_t i = 0; i < inputCount; i++)
		{
			// Root of 2 / input count
			float sigma = FMath::Sqrt(2.f / (float)inputCount);

			// Create gausian distribution with random device and zero mean
			std::random_device randdiv;
			std::mt19937 rangen(randdiv());
			std::normal_distribution<float> gaudis(0.0, sigma);

			// Add calculated init to weights and zero for the gradiant
			weights.Add(gaudis(rangen));
			gradiants_i_h.Add(0);
		}

		// Initialize weights
		w_I_H.Add(h, FWeightValues(weights, gradiants_i_h));
	}

	// Hidden -> Output
	for (size_t h = 0; h < outputCount; h++)
	{
		TArray<float> weights;
		TArray<float> gradiants_h_o;
		
		for (size_t i = 0; i < hiddenNeuronCount; i++)
		{
			// Normal Xaviar Init wegen output Sigmoid
			// https://365datascience.com/tutorials/machine-learning-tutorials/what-is-xavier-initialization/#h_24242636975541686829817569

			float xavstd = FMath::Sqrt(2.f / ((float)outputCount + (float)hiddenNeuronCount));

			std::random_device randdiv;
			std::mt19937 rangen(randdiv());
			std::normal_distribution<float> gaudis(0.0, xavstd);

			weights.Add(gaudis(rangen));
			gradiants_h_o.Add(0);
		}

		w_H_O.Add(h, FWeightValues(weights, gradiants_h_o));
	}


	// Pro layer
	TArray<FHHWeights> hhweights;

	for (size_t l = 0; l < hiddenLayerCount - 1; l++)
	{
		TArray<FWeightValues> wvals;

		for (size_t n = 0; n < hiddenNeuronCount; n++)
		{
			TArray<float> weights;
			TArray<float> gradiants_h_h;

			for (size_t nx = 0; nx < hiddenNeuronCount; nx++)
			{

				float sigma = FMath::Sqrt(2.f / (float)inputCount);

				std::random_device randdiv;
				std::mt19937 rangen(randdiv());
				std::normal_distribution<float> gaudis(0.0, sigma);

				weights.Add(gaudis(rangen));

				gradiants_h_h.Add(0);
			}

			wvals.Add(FWeightValues(weights, gradiants_h_h));
		}

		hhweights.Add(wvals);
	}

	int l = 0;
	for (FHHWeights hhw : hhweights)
	{
		w_H_H.Add(l, hhw);

		l++;
	}

	UE_LOG(LogTemp, Warning, TEXT("DDD"))
}

void AMarketPredictionSystem::InitBiases()
{
	TArray<float> biases;
	TArray<float> gradiant;

	for (size_t i = 0; i < hiddenNeuronCount; i++)
	{
		biases.Add(0);
		gradiant.Add(0);
	}

	b_I_H = (FHHBiases{ biases,gradiant });


	biases.Empty();
	gradiant.Empty();

	for (size_t i = 0; i < outputCount; i++)
	{
		biases.Add(0);
		gradiant.Add(0);
	}

	b_H_O = FHHBiases{ biases,gradiant };


	// -1 weil ich ja die biasis auf das nächste layer setzte
	for (size_t h = 0; h < hiddenLayerCount - 1; h++)
	{
		biases.Empty();
		gradiant.Empty();

		for (size_t i = 0; i < hiddenNeuronCount; i++)
		{
			biases.Add(0);
			gradiant.Add(0);

			b_H_H.Add(h, FHHBiases{ biases, gradiant });
		}
	}
}

TArray<FMiniBatchData> AMarketPredictionSystem::CreateMiniBatch()
{
	TArray<FMiniBatchData> minibatch;

	for (size_t batch = 0; batch < batchAmount; batch++)
	{
		TArray<FPlotInputOutputData> plots;

		for (size_t feature = 0; feature < batchSize; feature++)
		{
			FTableInputOutputData table = combinedTableDatas[FMath::RandRange(0, combinedTableDatas.Num() - 1)];
			FPlotInputOutputData plotdata = table.GetInputOutputPlots()[FMath::RandRange(0, table.GetInputOutputPlots().Num() - 1)];
			plots.Add(plotdata);
		}

		minibatch.Add(plots);
	}

	debugMiniBatchCache = minibatch;

	return minibatch;
}

void AMarketPredictionSystem::InitData()
{
	// startIDx from which to pull and remove the resource prices
	TArray<int> cullids;

	static int PLOT_END = 0;


	for (TTuple<int, FLoadedAndCuratedTable> table : loadedDataTables)
	{
		int plotidx = 0;

		FTableInputOutputData tabledata_sep;
		FTableInputOutputData tabledata_comb;

		for (FPlotData plot : table.Value.GetPlotData())
		{
			TArray<float> input;

			input.Add(plotidx);

			for (TTuple<EResourceIdent, FResourceMarketPoolInfo> resinfo : plot.GetResourceInfo())
			{
				input.Add((float)resinfo.Value.GetIdent());
				input.Add(resinfo.Value.GetAmount());
				input.Add(resinfo.Value.GetUnmodifiedPrice());

				cullids.Add(input.Num());

				input.Add(resinfo.Value.GetModifiedPrice());
				input.Add(resinfo.Value.GetDemandDelta());
				input.Add(resinfo.Value.GetDemandAlpha());
				input.Add(resinfo.Value.GetDemandLambda());
				input.Add(resinfo.Value.GetTimeSinceUpdate());
			}

			for (TTuple<int, FAITraderAttributeInfo> attributeinfo : plot.GetAllTraderAttributeInfos())
			{
				input.Add(attributeinfo.Key);

				for (TTuple<EResourceIdent, int> res : attributeinfo.Value.GetPrefSellResource())
				{
					input.Add((float)res.Key);
					input.Add(res.Value);
				}

				for (TTuple<EResourceIdent, int> res : attributeinfo.Value.GetPrefBuyResource())
				{
					input.Add((float)res.Key);
					input.Add((float)res.Value);
				}

				for (TTuple<EAIActions, float> state : attributeinfo.Value.GetPrefState())
				{
					input.Add((float)state.Key);
					input.Add((float)state.Value);
				}

				input.Add(attributeinfo.Value.GetAmountModifier());
				input.Add(attributeinfo.Value.GetEquilibriumAmount());
				input.Add(attributeinfo.Value.GetResourceGoalAmount());
				input.Add(attributeinfo.Value.GetCurrencyGoalAmount());
			}

			for (FTraderInformations traderinfo : plot.GetAllTraderInfos())
			{
				input.Add(traderinfo.GetIdent());
				input.Add(traderinfo.GetCurrency());

				for (TTuple<EResourceIdent, int> res : traderinfo.GetPouch())
				{
					input.Add((float)res.Key);
					input.Add(res.Value);
				}

				input.Add(traderinfo.GetWonWithRes());
				input.Add(traderinfo.GetWonWithCurr());
				input.Add(traderinfo.GetHasLost());
			}

			input.Add(plot.GetForexValue());

			PLOT_END = input.Num();


			TMap<int, FfloatArray> inputs_comb;
			TMap<int, FfloatArray> outputs_comb;

			TArray<float> inputdata;
			TArray<float> outputdata;

			int entryid = 0;
			int cullam = 0;

			for (float entry : input)
			{
				if (cullids.Contains(entryid))
				{
					outputdata.Add(entry);

					cullam++;
					entryid++;
					continue;
				}

				inputdata.Add(entry);
				entryid++;

				if (entryid % PLOT_END == 0)
					entryid = 0;
			}

			inputs_comb.Add(plotidx, FfloatArray(inputdata));
			outputs_comb.Add(plotidx, FfloatArray(outputdata));

			FPlotInputOutputData combdata =
			{
				outputs_comb,
				inputs_comb
			};

			tabledata_comb.AddPlot(plotidx, combdata);

			// duplicates with all informations
			TMap<int, FfloatArray> splitbyres;

			for (size_t i = 0; i < (int)EResourceIdent::RI_MAX_ENTRY - 1; i++)
			{
				FfloatArray farray =
				{
					input
				};

				splitbyres.Add(i, farray);
			}

			plotidx++;
		}

		combinedTableDatas.Add(tabledata_comb);
	}

	UE_LOG(LogTemp, Warning, TEXT("Data init Finished"));

	InitCombinedNetwork();
}

void AMarketPredictionSystem::InitCombinedNetwork()
{
	UE_LOG(LogTemp, Warning, TEXT("Creating neural network"))

	TArray<FNNNeuron> inputneurons;
	TArray<FNNNeuron> outputneurons;

	TMap<int, TArray<FNNNeuron>> hiddenln;

	for (size_t i = 0; i < combinedTableDatas[0].GetInputOutputPlots().FindRef(0).GetInputValues()[0].GetArray().Num(); i++)
	{
		inputneurons.Add(FNNNeuron(batchSize));
	}

	for (size_t i = 0; i < combinedTableDatas[0].GetInputOutputPlots().FindRef(0).GetOutput()[0].GetArray().Num(); i++)
	{
		outputneurons.Add(FNNNeuron(batchSize));
	}

	for (size_t l = 0; l < hiddenLayerCount; l++)
	{
		TArray<FNNNeuron> neurons;

		for (size_t n = 0; n < hiddenNeuronCount; n++)
		{
			neurons.Add(FNNNeuron(batchSize));
		}

		hiddenln.Add(l, neurons);
	}

	// Inits für die sclaes und shifts in abhängigkeit zu layer neuron count
	TArray<float> inputscale;
	TArray<float> inputshift;

	TArray<float> gshiftders_i;
	TArray<float> gscaleders_i;

	TArray<float> outputders_i;

	for (size_t i = 0; i < inputCount; i++)
	{
		inputscale.Add(1);
		inputshift.Add(0);

		gshiftders_i.Add(0);
		gscaleders_i.Add(0);

		outputders_i.Add(0);
	}

	TArray<float> hiddenscale;
	TArray<float> hiddenshift;

	TArray<float> gshiftders_h;
	TArray<float> gscaleders_h;

	for (size_t i = 0; i < hiddenNeuronCount; i++)
	{
		hiddenscale.Add(1);
		hiddenshift.Add(0);
	
		gshiftders_h.Add(0);
		gscaleders_h.Add(0);
	}


	inputCount = inputneurons.Num();
	outputCount = outputneurons.Num();

	URegularLayer* ilayer = NewObject<URegularLayer>(URegularLayer::StaticClass());
	ilayer->InitLayer_Regular(ELayerType::LT_Input, 0, inputneurons);
	neuralNetwork.Add(neuralNetwork.Num(), ilayer);

	UBatchNormLayer* iblayer = NewObject<UBatchNormLayer>(UBatchNormLayer::StaticClass());
	iblayer->InitLayer_Batch(ELayerType::LT_BatchNorm, 0,inputscale , inputshift, gscaleders_i, gshiftders_i);
	neuralNetwork.Add(neuralNetwork.Num(), iblayer);

	int lidx = 1;

	for (TTuple<int, TArray<FNNNeuron>> hidden : hiddenln)
	{
		URegularLayer* hl = NewObject<URegularLayer>(URegularLayer::StaticClass());
		hl->InitLayer_Regular(ELayerType::LT_Hidden, lidx, hidden.Value);
		neuralNetwork.Add(neuralNetwork.Num(), hl);

		if (lidx < hiddenLayerCount)
		{
			UBatchNormLayer* bl = NewObject<UBatchNormLayer>(UBatchNormLayer::StaticClass());
			bl->InitLayer_Batch(ELayerType::LT_BatchNorm, lidx, hiddenscale,hiddenshift , gscaleders_h, gshiftders_h);
			neuralNetwork.Add(neuralNetwork.Num(), bl);
		}
		
		lidx++;
	}

	UOutputLayer* ol = NewObject<UOutputLayer>(UOutputLayer::StaticClass());
	ol->InitLayer_Regular(ELayerType::LT_Output, lidx, outputneurons);

	neuralNetwork.Add(neuralNetwork.Num(), ol);

	UE_LOG(LogTemp, Warning, TEXT("Finished neural network with inputs: %i , hidden layer: %i, hidden neurons per layer: %i, outputs: %i"),
		inputneurons.Num(), hiddenln.Num(), hiddenln[0].Num(), outputneurons.Num());


	// Start first forwardpass
	FTimerHandle handle;
	GetWorld()->GetTimerManager().SetTimer(handle, this, &AMarketPredictionSystem::ForwardPass, 1.f, false);
}

void AMarketPredictionSystem::ForwardPass()
{
	FDateTime datetime;
	UKismetMathLibrary::GetTimeOfDay(datetime);


	TArray<FMiniBatchData> minibatch = CreateMiniBatch();

	int batchidx = 0;

	TArray<float> timearray;

	FBatchDebugData BATCHDEBUGDATA = CreateBatchDebugData(datetime);


	for (FMiniBatchData batch : minibatch)
	{
		SampleBatchDebugData(BATCHDEBUGDATA, datetime);

		// Ich setzte mir inputs und outputs über meine komplette mini batch und iterate dann in den individuellen gradiant calculations in jeden layer
		// regulär wie ich es auch vorher gemacht habe, nur halt über ALLE von der mini batch gesetzten daten, dann habe ich für jede mini batch einen
		// individuellen gradiant vektor welcher genutzt werden kann um forward zu propagioeren
		// So muss ich auch nichts über die batch data cachen, ich kann einfach alles weiter über das netzwerk laufen lassen

		// Ich setzte hier alle plots der mini batch IN dem NN
		SetInputsOutputs(batch.GetBatchData());

		int hhlayer = 0;

		for (size_t i = 0; i < hiddenLayerCount + 1; i++)
		{
			if (i > 0 && i % 2 == 0 || i == 0)
			{

				UNNLayer* currbl = BatchNormalization_F(batchidx, neuralNetwork[i], i + 1);
				URegularLayer* prevreglayer = Cast<URegularLayer>(neuralNetwork[i]);
				URegularLayer* currentreglayer = Cast<URegularLayer>(neuralNetwork[i + 2]);

				CalculateNeuronValues(currbl, i + 2, hhlayer);

				if (neuralNetwork[i]->GetLayerType() == ELayerType::LT_Hidden)
					hhlayer++;
			}
		}

		UNNLayer* unlayer = neuralNetwork[neuralNetwork.Num() - 2];
		int layeridx = neuralNetwork.Num() - 1;
		CalculateNeuronValues(unlayer, layeridx, hhlayer - 1);


		//MINIBATCHDEBUGDATA.AddSecondsElapsed(EElapsedCategories::EC_MeanSquaredError, datetime.Now().GetHour(), datetime.Now().GetMinute(), datetime.Now().GetSecond(), datetimestart);
		// Ich accumulate alle daten EINER mini batch und gehe dann in die backprop
		Backpropagation();


		// Hardcode für die letzte an diesem punkt
		BATCHDEBUGDATA.AddBatchEntry(CreateMiniBatchDebugData(BATCHDEBUGDATA, datetime));

		BATCHDEBUGDATA.AddTotalEnd(FDebugTime(datetime.Now().GetHour(), datetime.Now().GetMinute(), datetime.Now().GetSecond()));
		BATCHDEBUGDATA.GetTotalEnd()[BATCHDEBUGDATA.GetTotalEnd().Num() - 1].SetEnd(datetime.Now().GetHour(), datetime.Now().GetMinute(), datetime.Now().GetSecond());

		batchidx++;
	}

	epochDebugDatas.AddNewEpochData(BATCHDEBUGDATA);

	if (iteratedEpochs >= 5)
	{
		// Average time per batch 13.4 secs

		// batch last first pair
		TMap<int, TArray<int>> blfp;

		float avrg = 0;
		int idx = 0;

		for (TTuple<int, FBatchDebugData> dd : epochDebugDatas.GetEpochBatchPair())
		{
			TArray<int > bsep;

			for (int i = 0; i < dd.Value.GetTotalStart().Num(); ++i)
			{
				int sh = dd.Value.GetTotalStart()[i].GetHours();
				int sm = dd.Value.GetTotalStart()[i].GetMinutes();
				int ss = dd.Value.GetTotalStart()[i].GetSeconds();

				int st = (((sh * 60) + sm) * 60) + ss;

				int eh = dd.Value.GetTotalEnd()[i].GetHours();
				int em = dd.Value.GetTotalEnd()[i].GetMinutes();
				int es = dd.Value.GetTotalEnd()[i].GetSeconds();

				int et = (((eh * 60) + em) * 60) + es;

				et -= st;

				bsep.Add(et);
				avrg += et;
				idx++;
			}

			blfp.Add(blfp.Num(), bsep);
		}

		avrg /= (float)idx;
	}

	if (iteratedEpochs <= epochs)
	{
		iteratedEpochs++;
		ForwardPass();
	}
}

void AMarketPredictionSystem::Backpropagation()
{
	// Ich starte am ende des NN und arbeite mich dann zum Input zurück
	// In diesem fall ist dies halt folgend
	// Reg		->	Batch	== Output		->	Batch 2		 == CG_Reg_BatchNorm()	== BT_Hidden_Output
	// Batch	->	Reg		== Batch 2		->	Hidden 2	 == CG_BatchNorm_Reg()	
	// Reg		->	Batch	== Hidden 2		->	Batch 1		 == CG_Reg_BatchNorm()	== BT_Hidden_Hidden
	// Batch	->	Reg		== Batch 1		->	Hidden 1	 == CG_BatchNorm_Reg()	
	// Reg		->	Batch	== Hidden 1		->	Batch 0		 == CG_Reg_BatchNorm()	== BT_Input_Hidden
	// Reg		->	Batch	== Batch 0		->	Input		 == CG_BatchNorm_Reg()

	int posneg = 1;

	for (int i = neuralNetwork.Num() - 2; i >= 0; --i)
	{
		UNNLayer* currlayer = neuralNetwork[neuralNetwork.Num() - posneg];
		UNNLayer* prevlayer = neuralNetwork[neuralNetwork.Num() - (posneg + 1)];
		EWeightBiasType wbtype = EWeightBiasType::BT_DEFAULT;

		if (posneg % 2 == 0 || currlayer->GetLayerType() == ELayerType::LT_BatchNorm && prevlayer->GetLayerType() == ELayerType::LT_Hidden)
			wbtype = EWeightBiasType::BT_Hidden_Hidden;
		else if (i == neuralNetwork.Num() - 2)
			wbtype = EWeightBiasType::BT_Hidden_Output;
		else
			wbtype = EWeightBiasType::BT_Input_Hidden;

		CalculateGradiants(currlayer, prevlayer, wbtype, posneg);

		posneg++;
	}

	UpdateValuesByDerivatives();
}

void AMarketPredictionSystem::SetInputsOutputs(TArray<FPlotInputOutputData> _batchData)
{
	// Wenn ich 15 mal 15 mini batches habe´sollte ich für meine erste iteration
	// 15 * 287 inputs haben und
	// 15 * 3 outputs
	// Meine layer werden dann in der größe der mini batch scaliert
	// dies wird dann auch für die calculation auf layer eben voorgenommen
	// Ich gehe mit jeder mini batch durch ein jedes layer

	for (TTuple<int, UNNLayer*> layer : neuralNetwork)
	{
		// Das sollte mir dann meinen mini batch input output vector erstellen, sowohl mein inoput als auch mein output layer haben
		// beinhalten dann neuronen welche value vectoren in abhängigkkeit zu meinen mini batches besitzen

		if (layer.Value->GetLayerType() == ELayerType::LT_Input)
		{
			TArray<FNNNeuron> inputneurons;
			// Haben immer die selben dimensionen

			FPlotInputOutputData d = _batchData[0];
			TMap<int, FfloatArray> s = d.GetInputValues();
			int input_l = 0;

			for (TTuple<int, FfloatArray> rrr : s)
			{
				input_l = rrr.Value.GetArray().Num();
			}

			inputneurons.Init({ FNNNeuron() }, input_l);

			for (FPlotInputOutputData plot : _batchData)
			{
				for (TTuple<int, FfloatArray> values : plot.GetInputValues())
				{
					int nidx = 0;

					for (float value : values.Value.GetArray())
					{
						inputneurons[nidx].AddValue(value);
						nidx++;
					}
				}
			}

			Cast<URegularLayer>(layer.Value)->SetNeurons(inputneurons);
		}
		else if (layer.Value->GetLayerType() == ELayerType::LT_Output)
		{
			TArray<FNNNeuron> outputneurons;

			FPlotInputOutputData d = _batchData[0];
			TMap<int, FfloatArray> s = d.GetOutput();
			int output_l = 0;

			for (TTuple<int, FfloatArray> rrr : s)
			{
				output_l = rrr.Value.GetArray().Num();
			}

			outputneurons.Init({ FNNNeuron() }, output_l);

			for (FPlotInputOutputData plot : _batchData)
			{
				for (TTuple<int, FfloatArray> values : plot.GetOutput())
				{
					int nidx = 0;

					for (float value : values.Value.GetArray())
					{
						outputneurons[nidx].AddValue(value);
						nidx++;
					}
				}
			}

			UOutputLayer* ol = Cast<UOutputLayer>(layer.Value);
			ol->ResetOutput(batchSize, outputCount);
			ol->SetNeurons(outputneurons);
		}
	}
}

FBatchDebugData AMarketPredictionSystem::CreateBatchDebugData(FDateTime _time)
{
	FBatchDebugData BATCHDEBUGDATA;

	int worldsecond = _time.Now().GetHour() * 60;
	worldsecond += _time.Now().GetMinute();
	worldsecond *= 60;
	BATCHDEBUGDATA.startedAtWorldSecond = worldsecond;

	BATCHDEBUGDATA.structName_Saved = UKismetStringLibrary::Conv_Int64ToString(epochDebugDatas.GetEpochBatchPair().Num());
	BATCHDEBUGDATA.structName_Saved += "_n";
	BATCHDEBUGDATA.productionSiteID_Saved = UKismetStringLibrary::Conv_Int64ToString(epochDebugDatas.GetEpochBatchPair().Num());

	return  BATCHDEBUGDATA;
}

void AMarketPredictionSystem::SampleBatchDebugData(FBatchDebugData _bdd, FDateTime _time)
{
	int datetimestart = (((_time.Now().GetHour() * 60) + _time.Now().GetMinute()) * 60) + _time.Now().GetSecond();

	_bdd.AddTotalStart((FDebugTime(_time.Now().GetHour(), _time.Now().GetMinute(), _time.Now().GetSecond())));
	_bdd.GetTotalStart()[_bdd.GetTotalStart().Num() - 1].SetStart(_time.Now().GetHour(), _time.Now().GetMinute(), _time.Now().GetSecond());
}

FMiniBatchDebugData AMarketPredictionSystem::CreateMiniBatchDebugData(FBatchDebugData _bdd, FDateTime _time)
{
	FMiniBatchDebugData MINIBATCHDEBUGDATA;

	MINIBATCHDEBUGDATA.SetEndOfLast(EElapsedCategories::EC_Backpass, _time.Now().GetHour(), _time.Now().GetMinute(), _time.Now().GetSecond());

	MINIBATCHDEBUGDATA.structName_Saved = UKismetStringLibrary::Conv_Int64ToString(_bdd.GetIndexBatchPair().Num());
	MINIBATCHDEBUGDATA.structName_Saved += "_n";
	MINIBATCHDEBUGDATA.productionSiteID_Saved = UKismetStringLibrary::Conv_Int64ToString(_bdd.GetIndexBatchPair().Num());

	return MINIBATCHDEBUGDATA;
}

TMap<int, FfloatArray> AMarketPredictionSystem::TransformDataByBatch(UNNLayer* _layerToTransform)
{
	TArray<FNNNeuron> neurons = Cast<URegularLayer>(_layerToTransform)->GetNeurons();

	TMap<int, FfloatArray> featurevaluemap;

	for (size_t featureidx = 0; featureidx < neurons[0].GetValues().Num(); featureidx++)
	{
		TArray<float> transvals;

		for (FNNNeuron neuron : neurons)
		{
			transvals.Add(neuron.GetValues()[featureidx]);
		}

		featurevaluemap.Add(featureidx, transvals);
	}

	// Sollte mir das layer eign transformed von neurons x batch zu batch x neuron zurück geben (alle batches)
	return featurevaluemap;
}

TMap<int, FfloatArray> AMarketPredictionSystem::TransformDataByNeuron(UNNLayer* _layerToTransform)
{
	TMap<int, FfloatArray> batch = Cast<UBatchNormLayer>(_layerToTransform)->GetOutputs();

	TMap<int, FfloatArray> featurevaluemap;


	for (size_t neuronidx = 0; neuronidx < batch[0].GetArray().Num(); neuronidx++)
	{
		TArray<float> transvals;

		for (size_t featureidx = 0; featureidx < batch.Num(); featureidx++)
		{
			transvals.Add(batch[featureidx].GetArray()[neuronidx]);
		}

		featurevaluemap.Add(neuronidx, transvals);
	}

	// Sollte mir das layer eign transformed von neurons x batch zu batch x neuron zurück geben (alle batches)
	return featurevaluemap;
}

// https://www.youtube.com/watch?v=Jj_w_zOEu4M
UBatchNormLayer* AMarketPredictionSystem::BatchNormalization_F(int _batchIdx, UNNLayer* _prevLayer, int _layerIndex)
{
	TArray<FNNNeuron> neurons = Cast<URegularLayer>(_prevLayer)->GetNeurons();

	UBatchNormLayer* currbatchlayer = Cast<UBatchNormLayer>(neuralNetwork.FindRef(_layerIndex));

	TMap<int, FfloatArray> featurevaluemap;

	for (size_t featureidx = 0; featureidx < neurons[0].GetValues().Num(); featureidx++)
	{
		TArray<float> transvals;

		for (FNNNeuron neuron : neurons)
		{
			transvals.Add(neuron.GetValues()[featureidx]);
		}

		featurevaluemap.Add(featureidx, transvals);
	}

	TMap<int, FfloatArray> newouts;
	TMap<int, FfloatArray> normvalues;

	currbatchlayer->InitMeans(featurevaluemap.Num());
	currbatchlayer->InitVariants(featurevaluemap.Num());

	CalculateMean(featurevaluemap, currbatchlayer, featurevaluemap.Num());
	CalculateVariant(featurevaluemap, currbatchlayer, featurevaluemap.Num());

	float totalmean = CalculateTotalMean(currbatchlayer->GetMeans());
	float totalvariant = CalculateTotalVariant(currbatchlayer->GetVariants());

	for (TTuple<int, FfloatArray> feature : featurevaluemap)
	{
		TArray<float> normalizedvalues = NormalizeInputs(feature.Value.GetArray(), totalmean, totalvariant);

		normvalues.Add(feature.Key, normalizedvalues);
		
		TArray<float> ssvalues = ScaleAndShift(normalizedvalues, currbatchlayer);
		
		newouts.Add(feature.Key, ssvalues);
	}

	currbatchlayer->SetNewOutputs(newouts);
	currbatchlayer->SetNewNormalizedValues(normvalues);

	return currbatchlayer;
}

void AMarketPredictionSystem::BatchNormalization_B(UBatchNormLayer* _L1, URegularLayer* _Lm1)
{
	/*
		A. der. o. loss wrt output_i / 6.
		B. der. o. loss wrt input_i / 3.

		C. der. o. loss Gamma_i -> Scale / 2.
		D. der. o. loss beta_i -> Shift / 1.


		Um diese zu erschließen brauche ich;
		der. o. loss wrt mean_i / 5.
		der. o. loss wrt variance_i / 4.

		der. o. loss wrt intermediate_variables_i

		μ = 	mean
		σ^2 = 	variance

		y_i =  Input_i after Normalization, So after mean and variance aswell as Scaling, Shifting -> Output_i
		x^_i = Input_i after Normalization BUT before scale and shift
		β_i =  Beta_i
		γ_i =  Gamma_i

		loss = propagated loss of the network

		Written;
		1.
		------------------------------------------------------------
		der. o. loss wrt beta =
		sum over Features lenght !! (uncertain) and by batch dimension
		(
		der o. loss wrt der. y_i
		)

		2.
		------------------------------------------------------------
		der. o. loss wrt gamma =
		sum over feature lenght
		(
		der o. loss wrt der. y_i times x^_i
		)

		3.
		------------------------------------------------------------
		der. o. loss wrt der. x^_i =
		der. o. loss wrt der. y_i times gamma_i

		4.
		------------------------------------------------------------
		der o. loss wrt variance^2 =
		sum over feature size
		(
		der. o. loss wrt x^_i
		times (y_i - mean)
		times -1 / 2
		times (variance^2 + euler)^-3/2
		)

		5.
		------------------------------------------------------------
		der o. loss wrt mean =
		sum over feature size
		(
		der. o. loss wrt der x^_i
		times -1 / root(variance^2 + Euler

		plus der. o. loss wrt der o. variance^2
		times
		sum over feature size
		(
		-feature size times y_i - mean
		)
		/ BATCH SIZE
		)
		6.
		------------------------------------------------------------
		der. o. loss wrt y_i =
		der. o. loss wrt x^_i
		times 1 / root(variance^2 + euler)
		plus der. o. loss wrt. variance^2
		times 2 times (y_i - mean) / BATCH SIZE
		plus der. o. loss wrt der. mean
		times 1 / BATCH SIZE

		Gradiant;
		Derivative of the loss wrt.
		Beta is 1.
		Gamma is 2.

		the nomalized, unscaled and unshifted Input is 3.

		the variance is 4.
		the mean is 5.

		the normalized, scaled and shifted Input is 6.
	*/

	int featurelenght = _L1->GetOutputs()[0].GetArray().Num();
	int batchsize = _L1->GetOutputs().Num();

	// 1.
	// Batch -> feature -> Average
	FBBContainer total_der_by_batch_beta;

	for (int batchid = 0; batchid < _L1->GetOutputs().Num(); ++batchid)
	{
		TArray<float> der_beta;

		for (int featureid = 0; featureid < _L1->GetOutputs()[batchid].GetArray().Num(); ++featureid)
		{
			der_beta.Add(C_Der_Beta(_Lm1, featureid));
		}

		total_der_by_batch_beta.AddBBD(der_beta);
	}
	TArray<float> avrg_der_by_batch_beta = AverageDerivatives(featurelenght, batchsize, total_der_by_batch_beta);

	// 2.
	FBBContainer total_der_by_batch_gamma;

	for (int batchid = 0; batchid < _L1->GetOutputs().Num(); ++batchid)
	{
		TArray<float> der_gamma;

		for (int featureid = 0; featureid < _L1->GetOutputs()[batchid].GetArray().Num(); ++featureid)
		{
			der_gamma.Add(C_Der_Gamma(_L1, _Lm1, batchid, featureid, avrg_der_by_batch_beta));
		}

		total_der_by_batch_gamma.AddBBD(der_gamma);
	}
	TArray<float> avrg_der_by_batch_gamma = AverageDerivatives(featurelenght, batchsize, total_der_by_batch_gamma);

	// 3.
	FBBContainer total_der_batch_normval;
	for (int batchid = 0; batchid < _L1->GetOutputs().Num(); ++batchid)
	{
		TArray<float> der_normval;

		for (int featureid = 0; featureid < _L1->GetOutputs()[batchid].GetArray().Num(); ++featureid)
		{
			der_normval.Add(C_Der_NormVal(_L1, batchid, featureid));
		}

		total_der_batch_normval.AddBBD(der_normval);
	}
	TArray<float> avrg_der_by_batch_normval = AverageDerivatives(featurelenght, batchsize, total_der_batch_normval);

	// 4.
	FBBContainer total_der_batch_variance;
	for (int batchid = 0; batchid < _L1->GetOutputs().Num(); ++batchid)
	{
		TArray<float> der_variance;

		for (int featureid = 0; featureid < _L1->GetOutputs()[batchid].GetArray().Num(); ++featureid)
		{
			der_variance.Add(C_Der_Variance(_L1, batchid, featureid, avrg_der_by_batch_normval));
		}

		total_der_batch_variance.AddBBD(der_variance);
	}
	TArray<float> avrg_der_by_batch_variance = AverageDerivatives(featurelenght, batchsize, total_der_batch_variance);

	// 5.
	FBBContainer total_der_batch_mean;
	for (int batchid = 0; batchid < _L1->GetOutputs().Num(); ++batchid)
	{
		TArray<float> der_mean;

		for (int featureid = 0; featureid < _L1->GetOutputs()[batchid].GetArray().Num(); ++featureid)
		{
			der_mean.Add(C_Der_Mean(_L1, batchid, featureid, avrg_der_by_batch_normval, avrg_der_by_batch_variance, batchsize, featurelenght));
		}

		total_der_batch_mean.AddBBD(der_mean);
	}
	TArray<float> avrg_der_by_batch_mean = AverageDerivatives(featurelenght, batchsize, total_der_batch_mean);

	// 6.
	FBBContainer total_der_batch_out;
	for (int batchid = 0; batchid < _L1->GetOutputs().Num(); ++batchid)
	{
		TArray<float> der_out;

		for (int featureid = 0; featureid < _L1->GetOutputs()[batchid].GetArray().Num(); ++featureid)
		{
			der_out.Add(C_Der_Output(_L1, batchid, featureid, avrg_der_by_batch_normval, avrg_der_by_batch_variance, avrg_der_by_batch_mean, batchsize));
		}

		total_der_batch_out.AddBBD(der_out);
	}
	TArray<float> avrg_der_by_batch_out = AverageDerivatives(featurelenght, batchsize, total_der_batch_out);

	_L1->UpdateShiftDers(avrg_der_by_batch_beta);
	_L1->UpdateScaleDers(avrg_der_by_batch_gamma);

	_L1->SetPropGradByN(avrg_der_by_batch_out);
	//_L1->UpdateOutputDers(avrg_der_by_batch_out);
}

// Mit mini batch werde ich dann activation in der größe meiner mini batch mal den neurons haben
void AMarketPredictionSystem::CalculateNeuronValues(UNNLayer* _prevLayer, int _currLayerIndex, int _hhindex)
{
	URegularLayer* currentreglayer = Cast<URegularLayer>(neuralNetwork[_currLayerIndex]);

	bool bisbatch = false;

	TMap<int, FfloatArray> tdata;

	if (Cast<UBatchNormLayer>(_prevLayer))
		bisbatch = true;
	else
		tdata = TransformDataByBatch(_prevLayer);


	// Eine mini batch besteht aus mehreren trainigns samples, ich geh in meiner foreward durch jedes trainings set in meiner batch und	NICHT über alle sätze meiner mini batch gleichzeitig
	// Ich mein 32 war die feature zahl pro plot
	for (size_t batch = 0; batch < batchSize; batch++)
	{
		int neuronidx = -1;

		for (FNNNeuron neuron : currentreglayer->GetNeurons())
		{

			int weightidx = 0;
			FFloat32 sum = 0.f;

			float test = 0.f;

			neuronidx++;

			TArray<float> nvalues;

			if (bisbatch)
				nvalues = Cast<UBatchNormLayer>(_prevLayer)->GetOutputs()[batch].GetArray();
			else
				nvalues = tdata[batch].GetArray();

			for (FFloat32 val : nvalues)
			{
				FFloat32 weight = 0;
				FFloat32 bias = 0;

				if (_currLayerIndex == inputHiddenIndex)
				{
					weight = w_I_H[neuronidx].GetWeights()[weightidx];
					// Bias ist pro neuron, weight pro input
					bias = b_I_H.GetBiases()[neuronidx];
				}
				else if (_currLayerIndex >= hiddenOutputIndex)
				{
					// Ich glaube ich habe hier weight idx für beide genommen weil w_H_O und b_H_O die selben dimensionen haben?
					weight = w_H_O[neuronidx].GetWeights()[weightidx];
					bias = b_H_O.GetBiases()[neuronidx];
				}
				else
				{
					FHHWeights  ff = w_H_H[0];
					TArray<FWeightValues> dd = ff.GetWeightValues();
					TArray<float> cc = dd[neuronidx].GetWeights();

					weight = w_H_H[_hhindex].GetWeightValues()[neuronidx].GetWeights()[weightidx];
					bias = b_H_H[_hhindex].GetBiases()[neuronidx];
				}

				sum.FloatValue += weight.FloatValue * val.FloatValue + bias.FloatValue;

				weightidx++;
			}

			// To tuple for the comparison
			if (_currLayerIndex >= hiddenOutputIndex - 1)
				Cast<UOutputLayer>(neuralNetwork[_currLayerIndex])->SetCalculatedOutput(batch, neuronidx, Linear(sum.FloatValue));
			else
				Cast<URegularLayer>(neuralNetwork[_currLayerIndex])->UpdateNeuron(batch, neuronidx, LeakyRelu(sum.FloatValue));
		}
	}
}

void AMarketPredictionSystem::CalculateMean(TMap<int, FfloatArray> _neuronValues, UBatchNormLayer* _batchLayer, int _inputSize)
{
	for (size_t batch = 0; batch < _neuronValues.Num(); batch++)
	{
		float mean = 0.f;

		for (size_t feature = 0; feature < _neuronValues[0].GetArray().Num(); feature++)
		{
			mean += _neuronValues[batch].GetArray()[feature];
		}

		mean /= (float)_inputSize;

		_batchLayer->AddMeanToAll(batch, mean);
	}
}

void AMarketPredictionSystem::CalculateVariant(TMap<int, FfloatArray> _neuronValues, UBatchNormLayer* _batchLayer, int _inputSize)
{
	for (size_t batch = 0; batch < _neuronValues.Num(); batch++)
	{
		float variant = 0.f;
	
		for (size_t feature = 0; feature < _neuronValues[0].GetArray().Num(); feature++)
		{
			float vla = _neuronValues[batch].GetArray()[feature];
			float mean = _batchLayer->GetMeans()[batch];
			variant += FMath::Pow((vla - mean), 2);
		}
	
		variant /= (float)_inputSize;
	
		_batchLayer->AddVariantToAll(batch, variant);
	}
}

float AMarketPredictionSystem::CalculateTotalMean(TArray<float> _layerMeans)
{
	float totalmean = 0;

	for (float m : _layerMeans)
	{
		totalmean += m;
	}

	totalmean /= (float)_layerMeans.Num();

	return totalmean;
}

float AMarketPredictionSystem::CalculateTotalVariant(TArray<float> _layerVariants)
{
	float totalvariant = 0;

	for (float v : _layerVariants)
	{
		totalvariant += v;
	}
	totalvariant /= (float)_layerVariants.Num();
	totalvariant *= 2;

	return totalvariant;
}

TArray<float> AMarketPredictionSystem::NormalizeInputs(TArray<float> _neuronValues, float _totalMean, float _totalVariant)
{
	TArray<float> normalizedvalues;

	for (float in : _neuronValues)
	{
		// Ich hab die berechnung getestet, die sollte richtig sein
		normalizedvalues.Add((in - _totalMean) / FMath::Sqrt(_totalVariant + 0.00001f));
		// https://www.youtube.com/watch?v=Jj_w_zOEu4M&t=191s
	}

	return normalizedvalues;
}

TArray<float> AMarketPredictionSystem::ScaleAndShift(TArray<float> _normalizedvalues, UBatchNormLayer* _currbatchlayer)
{
	TArray<float> ssvalues;

	int idx = 0;

	for (float ni : _normalizedvalues)
	{
		float shift = _currbatchlayer->GetShifts()[idx];
		float scale = _currbatchlayer->GetScales()[idx];

		// Wenn ich noch keine trainierten werte habe nehme ich den in * 1 + 0
		ssvalues.Add(scale * ni + shift);
	}

	return ssvalues;
}

TArray<float> AMarketPredictionSystem::AverageDerivatives(int _featurLenght, int _batchLenght, FBBContainer _fbbCont)
{
	TArray<float> avrgders;

	for (int featureid = 0; featureid < _featurLenght; ++featureid)
	{
		float der_avrg = 0;

		for (int batchid = 0; batchid < _batchLenght; ++batchid)
		{
			der_avrg += _fbbCont.GetBBDer()[batchid][featureid];
		}

		der_avrg /= (float)_fbbCont.GetBBDer()[0].Num();
		avrgders.Add(der_avrg);
	}

	return avrgders;
}

float AMarketPredictionSystem::C_Der_Beta(UNNLayer* _Lm1, int _featureId)
{
	float der = _Lm1->GetSinglePropGrad(_featureId);

	return der;
}

float AMarketPredictionSystem::C_Der_Gamma(UBatchNormLayer* _L1, URegularLayer* _Lm1, int _batchId, int _featureId, TArray<float> _derBeta)
{
	float der = 0;

	float currval = _L1->GetOutputs()[_batchId].GetArray()[_featureId];
	float prevval = _Lm1->GetNeurons()[_featureId].GetValues()[_batchId];

	float der_beta = _derBeta[_featureId];

	der *= _L1->GetNormalizedVals()[_batchId].GetArray()[_featureId];

	return der;
}

float AMarketPredictionSystem::C_Der_NormVal(UBatchNormLayer* _L1, int _batchId, int _featureId)
{
	float der = 0;

	float currval = _L1->GetOutputs()[_batchId].GetArray()[_featureId];
	float gamma = _L1->GetScales()[_featureId];

	der = currval * gamma;

	return der;
}

float AMarketPredictionSystem::C_Der_Variance(UBatchNormLayer* _L1, int _batchId, int _featureId, TArray<float>  _derOut)
{
	float
	der = 0;

	float currderout = _derOut[_featureId];
	float outpostmean = currderout - _L1->GetMeans()[_batchId];
	float comp = -1.f / 2.f;
	float complmentvariance = FMath::Pow(_L1->GetVariants()[_batchId] + EULERS_NUMBER, (-3.f / 2.f));

	der = currderout * outpostmean * comp * complmentvariance;

	return der;
}

float AMarketPredictionSystem::C_Der_Mean(UBatchNormLayer* _L1, int _batchId, int _featureId, TArray<float> _derNormVal,
	TArray<float> _derVariance, int _batchSize, int _featureSize)
{
	float der = 0;

	float dernormval = _derNormVal[_featureId];
	float variancecomp = (-1.f / (float)UKismetMathLibrary::Sqrt((_L1->GetVariants()[_batchId]) + EULERS_NUMBER));
	float dervariance = _derVariance[_featureId];

	float fourthterm = 0;

	for (int featureid = 0; featureid < _featureSize; ++featureid)
	{
		float out = _L1->GetOutputs()[_batchId].GetArray()[_featureId];
		float mean = _L1->GetMeans()[_batchId];
		fourthterm += (-_featureSize * (out - mean));
	}
	fourthterm /= (float)_featureSize;

	der = ((dernormval * variancecomp) + dervariance * fourthterm) / (float)batchSize;

	return der;
}

float AMarketPredictionSystem::C_Der_Output(UBatchNormLayer* _L1, int _batchId, int _featureId, TArray<float> _derNormVal,
	TArray<float> _derVariance, TArray<float> _derMean, int _batchSize)
{
	float der = 0;

	float dernormval = _derNormVal[_featureId];
	float variancecomp = 1.f / (float)UKismetMathLibrary::Sqrt(_L1->GetVariants()[_batchId] + EULERS_NUMBER);
	float der_variance = _derVariance[_featureId];
	float outcomp = 2.f * (float)(_L1->GetOutputs()[_batchId].GetArray()[_featureId]) / (float)_batchSize;
	float der_mean = _derMean[_featureId];
	float batchcomp = 1.f / (float)_batchSize;

	der = dernormval * variancecomp + der_variance * outcomp + der_mean * batchcomp;

	return der;
}

float AMarketPredictionSystem::LeakyRelu(float _toActivate)
{
	float activated = _toActivate;

	if (_toActivate < 0)
		activated = slopeCoefficient * _toActivate;

	return activated;
}

float AMarketPredictionSystem::Linear(float _toActivate)
{
	return _toActivate;
}

float AMarketPredictionSystem::LinearDerivative()
{
	return 1;
}

float AMarketPredictionSystem::LeakyReLuDerivative(float _differentiationValue)
{
	float derivative = 1;

	if (_differentiationValue < 0)
		derivative = slopeCoefficient;

	return derivative;
}

void AMarketPredictionSystem::CalculateGradiants(UNNLayer* _L1, UNNLayer* _Lm1, EWeightBiasType _wbType, int _currLIndex, int _hiddenLIndex)
{
	TMap<int, FfloatArray> prevoutputs;

	int lml = 0;

	if (UBatchNormLayer* normlayer = Cast<UBatchNormLayer>(_Lm1))
	{
		prevoutputs = TransformDataByNeuron(normlayer);
		lml = normlayer->GetOutputs()[0].GetArray().Num();
	}
	else if (URegularLayer* reglayer = Cast<URegularLayer>(_Lm1))
	{
		lml = reglayer->GetNeurons().Num();
		int idx = 0;

		for (FNNNeuron neuron : reglayer->GetNeurons())
		{
			prevoutputs.Add(idx, neuron.GetValues());
			idx++;
		}
	}


	TMap<int, TArray<float>> input_derivative;
	TMap<int, TArray<float>> weight_derivative;

	// Jeder momentante neuron ist assoziirt mit einem weight zu einem jeden vorherigen neuron
	// also iterate ich für alle momentanen neurons über alle vorherigen

	// Calculate gradiants von vorherigem zum momentanen layer wenn das momentante layer regular is

	switch (_currLIndex)
	{
	case 1:
		ComputePropagatedError(EWeightBiasType::BT_Hidden_Output, _Lm1, lml);
		CG_Weight_Bias(_L1, _Lm1, EWeightBiasType::BT_Hidden_Output, _hiddenLIndex, prevoutputs);
		break;

	case 2:
		// Setzte hier für BL1
		ComputePropagatedError(EWeightBiasType::BT_Hidden_Hidden, _Lm1, lml);
		CG_Weight_Bias(_L1, _Lm1, EWeightBiasType::BT_Hidden_Hidden, _hiddenLIndex, prevoutputs);
		break;

	case 3:
		ComputePropagatedError(EWeightBiasType::BT_Hidden_Hidden, _Lm1, lml);
		BatchNormalization_B(Cast<UBatchNormLayer>(_L1), Cast<URegularLayer>(_Lm1));
		break;

	case 4:
		// Setzte hier für BL0
		ComputePropagatedError(EWeightBiasType::BT_Input_Hidden, _Lm1, lml);
		CG_Weight_Bias(_L1, _Lm1, EWeightBiasType::BT_Input_Hidden, _hiddenLIndex, prevoutputs);
		break;

	case 5:
		ComputePropagatedError(EWeightBiasType::BT_Input_Hidden, _Lm1, lml);
		BatchNormalization_B(Cast<UBatchNormLayer>(_L1), Cast<URegularLayer>(_Lm1));
		break;

	default:
		break;
	}

	ddebuglayerindex++;
}

// CurrLayer = L1
// PrevLayer = L-1
void AMarketPredictionSystem::CG_Weight_Bias(UNNLayer* _L1, UNNLayer* _Lm1, EWeightBiasType _wbType, int _hiddenLIndex, TMap<int, FfloatArray> _previousOutputs)
{
	// h0 == w_H_O, h1 == w_h_h, il == w_h_i  
	TArray<float> der_final_bias;

	// Derivative array by batch, should end up as batchsize * neuron.num() * output.num()
	TMap<int, TArray<float>> bias_derivatives_by_b;
	TArray<TMap<int, TArray<float>>> weight_derivatives_by_b;

	int featcount = 0;

	switch (_wbType)
	{

	case EWeightBiasType::BT_Input_Hidden:
	case EWeightBiasType::BT_Hidden_Hidden:
		featcount = Cast<URegularLayer>(_L1)->GetNeurons().Num();
		break;
	case EWeightBiasType::BT_Hidden_Output:
		featcount = Cast<UOutputLayer>(_L1)->GetNeurons().Num();
		break;

	case EWeightBiasType::BT_DEFAULT:
	case EWeightBiasType::BT_MAX_ENTRY:
		UE_LOG(LogTemp,Warning,TEXT("CG_Weight_Bias, BT_DEFAULT/BT_MAX_ENTRY"))
		break;
	default: ;
	}

	for (int batch = 0; batch < batchSize; ++batch)
	{
		EActivationFunction activFunk = EActivationFunction::AF_LeakyRelu;
		if (_wbType == EWeightBiasType::BT_Hidden_Output)
			activFunk = EActivationFunction::AF_Linear;

		// And calcout as the value for L-1
		FfloatArray lm1vals = {};
		FfloatArray l1vals = {};

 		TArray<float> propagatedGrads = _L1->GetAllPropGradsRads();

		URegularLayer* lm1r = Cast<URegularLayer>(_Lm1);

		if(lm1r)
		{
			for (int n = 0; n < lm1r->GetNeurons().Num(); ++n)
			{
				lm1vals.AddToArray(lm1r->GetNeurons()[n].GetValues()[batch]);
			}
		}
		else
		{
			UBatchNormLayer* lm1b = Cast<UBatchNormLayer>(_Lm1);

			for (int n = 0; n < lm1b->GetOutputs()[batch].GetArray().Num(); ++n)
			{
				lm1vals.AddToArray(lm1b->GetOutputs()[batch].GetArray()[n]);
			}
		}


		if(_wbType == EWeightBiasType::BT_Hidden_Output)
		{
			UOutputLayer* ol = Cast<UOutputLayer>(_L1);

			for (int n = 0; n < ol->GetNeurons().Num(); ++n)
			{
				l1vals.AddToArray(ol->GetCalcOutput()[n].GetValues()[batch]);
			}
		}
		else
		{
			URegularLayer* rl = Cast<URegularLayer>(_L1);

			for (int n = 0; n < rl->GetNeurons().Num(); ++n)
			{
				l1vals.AddToArray(rl->GetNeurons()[n].GetValues()[batch]);
			}
		}

		if(l1vals.GetArray().Num() <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("CG_Weight_Bias ,l1vals.GetArray().Num() <= 0"));
			return;
		}

		TMap<int, TArray<float>> weight_derivatives_by_n;

		for (int n_l1 = 0; n_l1 < featcount; ++n_l1)
		{
			TArray<float> der_vals_weight;
		
			// https://www.youtube.com/watch?v=tIeHLnjs5U8&t=311s
		
			// Ich iterate für jeden output um mir deren derivative so zu erschließen
			for (int n_lm1n = 0; n_lm1n < lm1vals.GetArray().Num(); ++n_lm1n)
			{
				// 1. a^L-1 // d(z^L)
				// 2. dz^L / dw^L // a^L1
				// 3. dCx / da^L // 2(a^L - y)

				// Performance über MSE ist erstmal raus -> Grad ka wofür ich das nutzen würde
				//if (_wbType == EWeightBiasType::BT_Hidden_Output)
				//	_currentLayer->AddMSEVlaue(ErrorGradiant(calouts.GetArray()[n_l1], exouts.GetArray()[n_l1], _wbType, _L1, _Lm1, n_l1));

				// propagatedGrads[n_l1])) weil ich den prop grad ja aus L1 nehme (Ich meine das war der fehler welche um die verrückung der weight lenght ein L nach rechts verantwortlich war)

				//float a = Der_ActivVal_Weight(lm1vals.GetArray()[n_l1]);
				//float b = Der_CurrVal_ActivVal(l1vals.GetArray()[n_l1], activFunk);
				//float c = propagatedGrads[n_l1];

				float a = Der_ActivVal_Weight(lm1vals.GetArray()[n_lm1n]);
				float b = Der_CurrVal_ActivVal(l1vals.GetArray()[n_l1], activFunk);
				float c = propagatedGrads[n_l1];

				der_vals_weight.Add(a * b * c);
			}
		
			weight_derivatives_by_n.Add(weight_derivatives_by_n.Num(), der_vals_weight);
		}
		
		TArray<float> der_vals_bias;
		
		for (int i = 0; i < l1vals.GetArray().Num(); ++i)
		{
			// 1. a^L-1 // d(z^L)
			// 2. dz^L / dw^L // a^L1
			// 3. dCx / da^L // 2(a^L - y)

			float a = 1.f;
			float b = Der_CurrVal_ActivVal(l1vals.GetArray()[i], activFunk);
			float c = propagatedGrads[i];

			der_vals_bias.Add(Der_ActivVal_Weight(a * b * c));
		}

		weight_derivatives_by_b.Add(weight_derivatives_by_n);
		bias_derivatives_by_b.Add(bias_derivatives_by_b.Num(), der_vals_bias);
	}

	if (_wbType == EWeightBiasType::BT_Hidden_Output)
		_L1->SetMSE((_L1->GetMSE() / (float)Cast<UOutputLayer>(_L1)->GetNeurons().Num()) / (float)batchSize);

	// averaged ders by neuron of L-1
	TMap<int, TArray<float>> der_final_weight;

	// Ich gehe über jeden neuron L-1
	// Ich erstell mir ein float array für die derivatives des momentanen neuron, er wird die länge L-1 haben
	// Ich gehe für die länge von L-1 über jede batch (Ich muss tuppeln weil ich L-1.Num() * L1.Num() habe)
	// Ich nehme mir das value der momentanen batch and der stelle des neurons
	// Ist dies abgeschlossen habe ich float array.num() == L-1.Num() += mit der jeweiligen iteration des neuron(x)L1
	// Am ende ost outders(x) == total aller vals über alle batches und wird averaged
	// Der avrg wird dann auf der_final_weight geadded
	// Ich hab am ende der_final_weight.Num() == L1.Num() x L-1.Num() (der_final_weight.Num() == L-1) und reverse die danach


	// Averaging
	for (int neuron = 0; neuron < weight_derivatives_by_b[0].Num(); ++neuron)
	{
		TArray<float> outders;

		for (int ders = 0; ders < weight_derivatives_by_b[0][0].Num(); ++ders)
		{
			outders.Add(0);
		}

		for (int ders = 0; ders < weight_derivatives_by_b[0][0].Num(); ++ders)
		{
			for (int batch = 0; batch < weight_derivatives_by_b.Num(); ++batch)
			{
				float v = weight_derivatives_by_b[batch][neuron][ders];

				outders[ders] += v;
			}
		}

		for (int i = 0; i < outders.Num(); ++i)
		{
			outders[i] /= (float)batchSize;
		}

		der_final_weight.Add(der_final_weight.Num(), outders);
	}

	for (int oini = 0; oini < bias_derivatives_by_b[0].Num(); ++oini)
	{
		der_final_bias.Add(0);
	}

	for (int ders = 0; ders < bias_derivatives_by_b[0].Num(); ++ders)
	{
		for (int batch = 0; batch < bias_derivatives_by_b.Num(); ++batch)
		{
			float v = bias_derivatives_by_b[batch][ders];

			der_final_bias[ders] += v;
		}
	}

	for (int i = 0; i < der_final_bias.Num(); ++i)
	{
		der_final_bias[i] /= (float)batchSize;
	}

	for (TTuple<int, TArray<float>> wd : der_final_weight)
	{
		// Immer hinterkopf behalten das die känge immer mit dem nächsten layer assoziirt ist, bsp.: 287 inputs m. 143 im nächsten layer bedeutet jeder neuron hat 287 weights im NÄCHSTEN (143x287)
		switch (_wbType) {
		case EWeightBiasType::BT_Input_Hidden:
			w_I_H[wd.Key].SetWeightGradiants(wd.Value);
			break;
		case EWeightBiasType::BT_Hidden_Output:
			w_H_O[wd.Key].SetWeightGradiants(wd.Value);
			break;
		case EWeightBiasType::BT_Hidden_Hidden:
		{
			// Ich mein das wird immer 0 sein, da ich im moment nur mit 2 hidden layern arbeite ein float array aus float arrays brauchte
			TArray<FWeightValues> vals;
			w_H_H[0].SetGradiantByIndex(wd.Key, wd.Value);
			break;
		}
		case EWeightBiasType::BT_DEFAULT:
		case EWeightBiasType::BT_MAX_ENTRY:
		default:
			break;
		}
	}

	switch (_wbType) {
	case EWeightBiasType::BT_Input_Hidden:
		b_I_H.SetBiasGradiants(der_final_bias);
		break;
	case EWeightBiasType::BT_Hidden_Output:
		b_H_O.SetBiasGradiants(der_final_bias);
		break;
	case EWeightBiasType::BT_Hidden_Hidden:
		// 0 weil ich hab nur ein hidden Layer im moment
		b_H_H[0].SetBiasGradiants(der_final_bias);
		break;

	case EWeightBiasType::BT_DEFAULT:
	case EWeightBiasType::BT_MAX_ENTRY:
	default:
		break;
	}
}

void AMarketPredictionSystem::ComputePropagatedError(EWeightBiasType _wbType, UNNLayer* _Lm1, int _LmLenght)
{
	TMap<int, TArray<float>> gradbybatch;
	UOutputLayer* _l1 = Cast<UOutputLayer>(neuralNetwork[neuralNetwork.Num() - 1]);

	for (int batch = 0; batch < batchSize; ++batch)
	{
		FfloatArray expected;
		FfloatArray calculated;

		for (int on = 0; on < _l1->GetNeurons().Num(); ++on)
		{
			expected.AddToArray(_l1->GetNeurons()[on].GetValues()[batch]);
		}

		for (int n = 0; n < _LmLenght; ++n)
		{
			if (URegularLayer* rl = Cast<URegularLayer>(_Lm1))
				calculated.AddToArray(rl->GetNeurons()[n].GetValues()[batch]);
			else if (UBatchNormLayer* bl = Cast<UBatchNormLayer>(_Lm1))
				calculated.AddToArray(bl->GetOutputs()[batch].GetArray()[n]);


		}

		gradbybatch.Add(gradbybatch.Num(), GetPropagatedGradiants(_wbType, _l1, _Lm1, batch, calculated, expected));


	}

	TArray<float> avrggrad;

	for (int n = 0; n < gradbybatch[0].Num(); ++n)
	{
		float avrg = 0;
		for (int b = 0; b < gradbybatch.Num(); ++b)
		{
			avrg += gradbybatch[b][n];
		}

		float av = avrg / (float)batchSize;
		avrggrad.Add(avrg);
	}

	if(_wbType == EWeightBiasType::BT_Hidden_Output)
		_l1->SetPropGradByN(avrggrad);

	_Lm1->SetPropGradByN(avrggrad);
}


float AMarketPredictionSystem::ErrorGradiant(float _currBatchOuts_calc, float _currBatchOuts_ex, EWeightBiasType _wbType, UNNLayer* _L1, UNNLayer* _Lm1, int _L1idx)
{
	//https://www.youtube.com/watch?v=Zr5viAZGndE

	float cost = 0;

	if (_wbType == EWeightBiasType::BT_Hidden_Output)
	{
		cost = 2.f * (_currBatchOuts_calc - _currBatchOuts_ex);
	}
	else
	{
		for (float grads : _L1->GetAllPropGradsRads())
		{
			cost += grads;
		}

		cost /= (float)_L1->GetAllPropGradsRads().Num();
	}

	return cost;
}

TArray<float> AMarketPredictionSystem::GetPropagatedGradiants(EWeightBiasType _wbType, UNNLayer* _L1, UNNLayer* _Lm1, int _batch, FfloatArray _calcOuts, FfloatArray _expectedOuts)
{
	TArray<float> gradbyl1m;

	for (int n_l1 = 0; n_l1 < _calcOuts.GetArray().Num(); ++n_l1)
	{
		TArray<float> prop;
		for (int n_lmn = 0; n_lmn < _expectedOuts.GetArray().Num(); ++n_lmn)
		{
			prop.Add(ErrorGradiant(_calcOuts.GetArray()[n_l1], _expectedOuts.GetArray()[n_lmn], _wbType, _L1, _Lm1, n_l1));
		}

		float avrg = 0;
		for(float total : prop)
		{
			avrg += total;
		}
		avrg /= (float)prop.Num();

		gradbyl1m.Add(avrg);
	}

	return gradbyl1m;
}

float AMarketPredictionSystem::Der_CurrVal_ActivVal(float _l1NeuronVal, EActivationFunction _activationFunc)
{
	float der_currval_activval = 0;

	// mein _neuron ist ja der welchen ich grade habe, ich will jetzt für ihn über den count aller hier rüber gehen (mein ich auf jeden fall
	// -> for neurons.Num()

	switch (_activationFunc)
	{
	case EActivationFunction::AF_Linear:
		der_currval_activval = LinearDerivative();
		break;
	case EActivationFunction::AF_LeakyRelu:
		der_currval_activval = LeakyReLuDerivative(_l1NeuronVal);
		break;


	case EActivationFunction::AF_DEFAULT:
	case EActivationFunction::AF_MAX_ENTRY:
	default:
		break;
	}

	return der_currval_activval;
}

float AMarketPredictionSystem::Der_ActivVal_Weight(float _prevNeuronVal)
{
	float der_activval_weight = _prevNeuronVal;

	return der_activval_weight;
}

void AMarketPredictionSystem::UpdateValuesByDerivatives()
{
	UpdateWeights();
	UpdateBiasis();
	UpdateBatchLayers();
}

void AMarketPredictionSystem::UpdateWeights()
{
	for (TTuple<int, FWeightValues> w : w_I_H)
	{
		int wvidx = 0;
		TArray<float> newweights;

		for (float currweight : w.Value.GetWeights())
		{
			float der = w.Value.GetWeightGradiants()[wvidx];
			float newva = currweight - (learningRate * der);

			float newval = newva;

			if (newval == 0)
				UE_LOG(LogTemp, Warning, TEXT("DD"))

				wvidx++;
		}
		w.Value.SetNewWeights(newweights);
	}

	for (TTuple<int, FWeightValues> w : w_H_O)
	{
		int wvidx = 0;
		TArray<float> newweights;

		for (float currweight : w.Value.GetWeights())
		{
			float der = w.Value.GetWeightGradiants()[wvidx];
			float newval = currweight - (learningRate * der);

			newweights.Add(newval);

			if (newval == 0)
				UE_LOG(LogTemp, Warning, TEXT("DD"))

				wvidx++;
		}
		w.Value.SetNewWeights(newweights);
	}


	for (TTuple<int, FHHWeights> w : w_H_H)
	{
		for (FWeightValues hl : w.Value.GetWeightValues())
		{
			int wvidx = 0;
			TArray<float> newweights;

			for (float currweight : hl.GetWeights())
			{
				float der = hl.GetWeightGradiants()[wvidx];
				float newval = currweight - (learningRate * der);

				newweights.Add(newval);

				if (newval == 0)
					UE_LOG(LogTemp, Warning, TEXT("DD"))

					wvidx++;
			}
			hl.SetNewWeights(newweights);
		}
	}
}

void AMarketPredictionSystem::UpdateBiasis()
{
	TArray<float> newbias_ih;
	for (int i = 0; i < b_I_H.GetBiases().Num(); ++i)
	{
		float currb = b_I_H.GetBiases()[i];
		float der = b_I_H.GetBiasGradiants()[i];
		float newval = currb - (learningRate * der);

		newbias_ih.Add(newval);
	}
	b_I_H.SetNewBiasis(newbias_ih);

	TArray<float> newbias_ho;
	for (int i = 0; i < b_H_O.GetBiases().Num(); ++i)
	{
		float currb = b_H_O.GetBiases()[i];
		float der = b_H_O.GetBiasGradiants()[i];
		float newval = currb - (learningRate * der);

		newbias_ho.Add(newval);
	}
	b_H_O.SetNewBiasis(newbias_ho);

	TArray<float> newbias_hh;
	for (TTuple<int, FHHBiases> hl : b_H_H)
	{
		for (int i = 0; i < hl.Value.GetBiases().Num(); ++i)
		{
			float currb = b_H_H[hl.Key].GetBiases()[i];
			float der = b_H_H[hl.Key].GetBiasGradiants()[i];
			float newval = currb - (learningRate * der);

			newbias_hh.Add(newval);
		}
		b_H_H[hl.Key].SetNewBiasis(newbias_hh);
	}
}


void AMarketPredictionSystem::UpdateBatchLayers()
{
	for (TTuple<int, UNNLayer*> l : neuralNetwork)
	{
		if (UBatchNormLayer* blayer = Cast<UBatchNormLayer>(l.Value))
		{
			TArray<float> newgammas;
			TArray<float> newbetas;

			for (int i = 0; i < blayer->GetScaleGradiants().Num(); ++i)
			{
				float newg = blayer->GetScales()[i] - (learningRate * blayer->GetScaleGradiants()[i]);
				newgammas.Add(newg);

				float newb = blayer->GetShifts()[i] - (learningRate * blayer->GetShiftGradiants()[i]);
				newbetas.Add(newb);
			}

			blayer->SetNewScales(newgammas);
			blayer->SetNewShifts(newbetas);
		}
	}
}