
// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionDatabase.h"

#include "ForexCurve.h"
#include "Curves/CurveVector.h"
#include "Engine/Private/DataTableCSV.h"
#include "AIPlayer.h"
#include "SaveGameManager.h"
#include "SplinePath.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetTextLibrary.h"

AActionDatabase::AActionDatabase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AActionDatabase::BeginPlay()
{
	Super::BeginPlay();
}

void AActionDatabase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AActionDatabase::InitActionDatabase(FActionDatabaseSaveData _savedata, ASaveGameManager* _saveGameManager, bool _bEnableExpoSmoothing)
{
	defaultSaveDir = UKismetSystemLibrary::GetProjectDirectory();
	saveGameManager = _saveGameManager;

	bEnableExponentialSmoothing = _bEnableExpoSmoothing;

	if(_savedata.WasSaveInit())
		loadedData = _savedata.GetLoadededForexData();
	else
	{
		if (defaultSaveDir.Len() <= 0)
		{
			UE_LOG(LogTemp,Error,TEXT("AActionDatabase, defaultSaveDir.Len() <= 0"))
			return;
		}
			
		loadedData = LoadForexDataFromFile(defaultSaveDir, defaultFileName);
	}
}

FActionDatabaseSaveData AActionDatabase::GetActionDataBaseSaveData()
{
	FActionDatabaseSaveData newsave =
	{
		loadedData
	};

	return newsave;
}

bool AActionDatabase::SaveDataToFile(FString _saveDir, FString _fileName, bool _bAllowOverWriting, TArray<FPlotData> _plotArrayToSave)
{
	_saveDir += "\\Content";
	_saveDir += "\\MarketData";
	_saveDir += "\\";
	_saveDir += _fileName;

	if(!_bAllowOverWriting)
	{
		if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*_saveDir))
			return false;
	}

	FString finalstring = "";

	finalstring += "plotindex";
	finalstring += LINE_TERMINATOR;
	finalstring += "resid,resamount,resunmodprice,resmodprice,resdelta,resalpha,reslambda,timesinceresupdate";
	finalstring += LINE_TERMINATOR;
	finalstring += "traderid,preffsellresident,preffsellresvalue,preffbuyresident,prefbuyresvalue,preffstateident,preffstatevalue,amountmod,equilibriumamount,resgoalamount,currgoalamount";
	finalstring += LINE_TERMINATOR;
	finalstring += "traderid,currency,resident,resamount,wonwithres,wonwithcurr,haslost";
	finalstring += LINE_TERMINATOR;
	finalstring += "forexweight";
	finalstring += LINE_TERMINATOR;

	int plotnum = 0;

	for(FPlotData data : _plotArrayToSave)
	{
		finalstring += FString::FromInt(plotnum);
		finalstring += ',';
		finalstring += LINE_TERMINATOR;

		// Market Resource Info:
		finalstring += CreateMarketInfoString(data);
		finalstring += LINE_TERMINATOR;
		finalstring += CreateAttributeInfoString(data);
		finalstring += LINE_TERMINATOR;
		finalstring += CreateAIInfoString(data);
		finalstring += LINE_TERMINATOR;
		finalstring += CreateForexInfo(data);

		finalstring += LINE_TERMINATOR;
		plotnum++;
	}

	return FFileHelper::SaveStringToFile(finalstring, *_saveDir);
}

TArray<FForexPlotData> AActionDatabase::LoadForexDataFromFile(FString _saveDir, FString _fileName)
{
	_saveDir += "\\";
	_saveDir += _fileName;

	TArray<FString> uncuratedloaded;

	FFileHelper::LoadFileToStringArray(uncuratedloaded, *_saveDir);

	int entryidx = 0;

	TArray<FForexPlotData> curatedforexdata;

	for(FString entry : uncuratedloaded)
	{
		bool culldate = false;
		FString newvalue;

		int idx = 0;
		int leng = entry.Len();

		TArray<float> entries;

		for(char character : entry)
		{
			idx++;

			if (character != ',' && culldate)
				newvalue += character;

			if (character == ',' && !culldate)
				culldate = true;
			else if(culldate && character == ',' || culldate && idx >= leng)
			{
				entries.Add(UKismetStringLibrary::Conv_StringToFloat(newvalue));

				newvalue.Empty();
			}
		}

		FForexPlotData newplot = 
		{
			entryidx,
			entries[0],
			entries[1],
			entries[2],
			entries[3],
			entries[4]
		};

		curatedforexdata.Add(newplot);

		entryidx++;
	}

	return curatedforexdata;
}

void AActionDatabase::LoadCSVFromFile(FString _saveDir, FString _fileName, int _tableAmount)
{
	_saveDir += "\\";
	_saveDir += _fileName;

	TArray<FString> loadeddata;

	FFileHelper::LoadFileToStringArray(loadeddata, *_saveDir);

	int entryidx = 0;

	TArray<FLoadedDataRow> tableindexrowmap;

	for (FString entry : loadeddata)
	{
		if(entryidx <= DataRuleSet.GetStartIdx())
		{
			entryidx++;
			continue;
		}

		// To write out the read data and then convert them
		FString newvalue = "";
		TArray<float> rowentries;

		int column = 0;

		bool bwasempty = false;

		if (entry.GetCharArray().Num() <= 0)
			bwasempty = true;

		for (char character : entry)
		{
			if (character != ',')
				newvalue += character;
			else if(character == ',')
			{
				// new row entry
				float valuentry = UKismetStringLibrary::Conv_StringToFloat(newvalue);
				rowentries.Add(valuentry);

				newvalue.Empty();

				column++;
			}
		}

		if(bwasempty)
			continue;

		FLoadedDataRow newrow =
		{
			rowentries
		};

		tableindexrowmap.Add( newrow);
		entryidx++;
	}

	FLoadedDataTable newtable =
	{
		tableindexrowmap
	};

	allLoadedTables.Add(newtable);

	if (allLoadedTables.Num() >= _tableAmount)
		CurateLoadedData();
}


void AActionDatabase::PlotMarketSituation(FPlotData _plotData)
{
	allPlottedData.Add(_plotData);

	if(allPlottedData.Num() % FOCET_INTERVAL == 0)
	{
		for(AAIPlayer* ai : saveGameManager->GetAllAIPlayer())
		{
			if(ai)
				ai->InternalFocet();
		}
	}
}

void AActionDatabase::CurateLoadedData()
{
	// Entire id (0-max)
	int rowid = 0;
	// Individual plot (0-29)
	int plotid = 0;

	TArray<FCuratedTable> allcuratedTables;

	for (FLoadedDataTable table : allLoadedTables)
	{
		TArray<FCuratedDataRow> curatedrows;

		for(FLoadedDataRow datarow : table.GetAllTableRows())
		{
			EDataCategory category = EDataCategory::DC_Index;

			if (plotid >= DataRuleSet.GetCatSetById().Find(EDataCategory::DC_Index)->GetMin() && plotid <= DataRuleSet.GetCatSetById().Find(EDataCategory::DC_Index)->GetMax())
				category = EDataCategory::DC_Index;
			else if (plotid >= DataRuleSet.GetCatSetById().Find(EDataCategory::DC_MarketResourceInfo)->GetMin() && plotid <= DataRuleSet.GetCatSetById().Find(EDataCategory::DC_MarketResourceInfo)->GetMax())
				category = EDataCategory::DC_MarketResourceInfo;
			else if (plotid >= DataRuleSet.GetCatSetById().Find(EDataCategory::DC_AIAttributeInfo)->GetMin() && plotid <= DataRuleSet.GetCatSetById().Find(EDataCategory::DC_AIAttributeInfo)->GetMax())
				category = EDataCategory::DC_AIAttributeInfo;
			else if (plotid >= DataRuleSet.GetCatSetById().Find(EDataCategory::DC_AIInfo)->GetMin() && plotid <= DataRuleSet.GetCatSetById().Find(EDataCategory::DC_AIInfo)->GetMax())
				category = EDataCategory::DC_AIInfo;
			else if (plotid >= DataRuleSet.GetCatSetById().Find(EDataCategory::DC_ForexDataInfo)->GetMin() && plotid <= DataRuleSet.GetCatSetById().Find(EDataCategory::DC_ForexDataInfo)->GetMax())
				category = EDataCategory::DC_ForexDataInfo;

			curatedrows.Add(FCuratedDataRow(category, CreateNameValuePair(category, datarow.GetRowEntries())));

			plotid++;
			rowid++;

			if (plotid >= plotLenght)
				plotid = 0;
		}

		FCuratedTable newcuratedtable = { curatedrows };

		allcuratedTables.Add(newcuratedtable);
	}

	ReconstructAllTables(allcuratedTables);

	int plotsize = SampleDataSizePerPlot(allcuratedTables[0]);

	TMap<int, float> valuesbyplot;

	TMap<int, FCuratedPlot> tabledatabyplot;

	for(FCuratedTable table : allcuratedTables)
	{
		TArray<float> datatoaverage;

		for(FCuratedDataRow row : table.GetCuratedRowEntries())
		{
			// For the average curve
			for(TTuple<EDataName, float> column : row.GetRowNameValuePairs())
			{
				if(column.Key == dataToAverage)
				{
					datatoaverage.Add(column.Value);
					break;
				}
			}
		}

		int iteratoridx = 0;
		int posinplot = 0;

		TArray<float> plotdata;

		for(float data : datatoaverage)
		{
			// Take iterator value from plot
			float current = 0;
			if(tabledatabyplot.Num() > 0 && tabledatabyplot.FindRef(posinplot).GetPlotData().Num() > iteratoridx)
				current = tabledatabyplot.FindRef(posinplot).GetPlotData()[iteratoridx];
			
			float newvalue = current + data;

			plotdata.Add(newvalue);

			iteratoridx++;

			if(iteratoridx >= plotsize)
			{
				tabledatabyplot.Add(posinplot, plotdata);
				plotdata.Empty();

				// iteration trough one plot
				posinplot++;
				iteratoridx = 0;
			}
		}
	}

	MapAverageData(tabledatabyplot);
}

TArray<float> AActionDatabase::SimpleExponentialSmoothing(TArray<float> data, float alpha)
{
	TArray<float> smoothed;

	smoothed.Add(data[0]);

	for (size_t h = 1; h < data.Num(); h++)
	{
		float currsmoth = alpha * data[h] + (1 - alpha) * smoothed[h - 1];
		smoothed.Add(currsmoth);
	}

	float lastsmoth = smoothed[smoothed.Num() - 1];
	float lastdata = data[data.Num() - 1];


	float currsmoth = alpha * lastdata + (1 - alpha) * lastsmoth;

	smoothed.Add(currsmoth);

	return smoothed;
}

void AActionDatabase::MapAverageData(TMap<int, FCuratedPlot> _tableDataByPlot)
{
	UMarketCurve* averagecurve = NewObject<UMarketCurve>(this);

	TMap<int, FPlotData> totalperplot;

	for(TTuple<int, FLoadedAndCuratedTable> table : allLoadedTables_Reconstructed)
	{
		int plotid = 0;

		for(FPlotData plot : table.Value.GetPlotData())
		{
			TMap<EResourceIdent, FResourceMarketPoolInfo> newresplot;
			
			for(TTuple<EResourceIdent, FResourceMarketPoolInfo> res : plot.GetResourceInfo())
			{
				// average per plot
				float newval = 0;

				if(totalperplot.Num() > plotid)
					newval = totalperplot[plotid].GetResourceInfo().FindRef(res.Key).GetModifiedPrice();

				newval += res.Value.GetModifiedPrice();

				FResourceMarketPoolInfo newresinfo =
				{
					res.Value.GetUnmodifiedPrice(),
					(float)newval,
					res.Value.GetAmount(),
					res.Value.GetDemandDelta(),
					res.Value.GetDemandAlpha(),
					res.Value.GetDemandLambda(),
					res.Value.GetTimeSinceUpdate(),
					res.Value.GetIdent()
				};


				newresplot.Add(res.Key, newresinfo);
			}


			FPlotData newplot =
			{
				newresplot,
				plot.GetAllTraderAttributeInfos(),
				plot.GetAllTraderInfos(),
				plot.GetForexValue()
			};


			totalperplot.Add(plotid, newplot);

			plotid++;
		}

	}

	int plotid = 0;
	TArray<float> averages;

	for(TTuple<int, FPlotData> plot : totalperplot)
	{
		TMap<EResourceIdent, FResourceMarketPoolInfo> newresinfo; 

		for(TTuple<EResourceIdent, FResourceMarketPoolInfo> res : plot.Value.GetResourceInfo())
		{
			float avrg = res.Value.GetModifiedPrice() / allLoadedTables_Reconstructed.Num();
			
			switch (res.Key)
			{
			case EResourceIdent::RI_Stone:		
				averagecurve->floatCurves[0].UpdateOrAddKey(plotid, avrg);
				if (bEnableExponentialSmoothing)
					averages.Add(avrg);
				break;

			case EResourceIdent::RI_Water:		
				if(!bEnableExponentialSmoothing)
				averagecurve->floatCurves[1].UpdateOrAddKey(plotid, avrg);
				break;
			case EResourceIdent::RI_Wood:		
				if(!bEnableExponentialSmoothing)
				averagecurve->floatCurves[2].UpdateOrAddKey(plotid, avrg);
				break;

			case EResourceIdent::RI_MAX_ENTRY: 
			case EResourceIdent::RI_DEFAULT:
			default: break;
			}

			FResourceMarketPoolInfo avrgres =
			{
				res.Value.GetUnmodifiedPrice(),
				(float)avrg,
				res.Value.GetAmount(),
				res.Value.GetDemandDelta(),
				res.Value.GetDemandAlpha(),
				res.Value.GetDemandLambda(),
				res.Value.GetTimeSinceUpdate(),
				res.Value.GetIdent()
			};

			newresinfo.Add(res.Key, avrgres);
		}

		TMap<int, FAITraderAttributeInfo> traderattinfo = plot.Value.GetAllTraderAttributeInfos();
		TArray<FTraderInformations> traderinfo = plot.Value.GetAllTraderInfos();
		float forex = plot.Value.GetForexValue();

		FPlotData avrgplot =
		{
			newresinfo,
			traderattinfo,
			traderinfo,
			forex
		};

		averagePlotData.Add(avrgplot);

		plotid++;
	}

	TArray<float> smoothvals_avrg;

	if(bEnableExponentialSmoothing)
	{
		int idx = 0;
		TArray<float> plotcache;

		for (float av : averages)
		{
			TArray<float> smoothvals;

			if (idx % exponentialSmoothStepLenght == 0)
			{
				if (idx <= 0)
				{
					TArray<float> e;
					e.Add(av);
					smoothvals = SimpleExponentialSmoothing(e, exponentialSmoothAlpha);
				}
				else
					smoothvals = SimpleExponentialSmoothing(plotcache, exponentialSmoothAlpha);

				averagecurve->floatCurves[1].UpdateOrAddKey(idx, smoothvals.Last());
				smoothvals_avrg.Add(smoothvals.Last());

				plotcache.Empty();
			}

			plotcache.Add(av);

			idx++;

			if (idx >= averages.Num())
			{
				float lp = smoothvals_avrg[smoothvals_avrg.Num() - 1];
				smoothvals_avrg.Add(lp);
			}
		}
	}

	curve = averagecurve;

	ASplinePath* sp = GetWorld()->SpawnActor<ASplinePath>(pathClass,GetActorLocation(), FRotator());

	sp->DrawGraph(averages, false);
	sp->DrawGraph(smoothvals_avrg, true);

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);

	SaveDataToFile(UKismetSystemLibrary::GetProjectDirectory(), "Average_Table", false, averagePlotData);
}

void AActionDatabase::ReconstructAllTables(TArray<FCuratedTable> _allToReconstruct)
{
	for(FCuratedTable table : _allToReconstruct)
	{
		int plotpos = 0;

		TArray<FPlotData> tablePlots;

		// All values of one PLOT 
		TMap<EResourceIdent, FResourceMarketPoolInfo> plotresinfos;
		TMap<int, FAITraderAttributeInfo> plottraderattinfos;
		TArray<FTraderInformations> plottraderinfos;
		float plotforexvalue = 0;

		for(FCuratedDataRow plotrow : table.GetCuratedRowEntries())
		{
			switch (plotrow.GetRowCategory())
			{
				// Single value entries should always be in the first column 
				case EDataCategory::DC_Index:					 plotpos = UKismetMathLibrary::FFloor(plotrow.GetRowNameValuePairs().begin().Value());															break;
				case EDataCategory::DC_MarketResourceInfo:	 plotresinfos.Add(ReconResourceRow(plotpos, plotrow).begin().Key(), ReconResourceRow(plotpos, plotrow).begin().Value());					    break;
				case EDataCategory::DC_AIAttributeInfo:		 plottraderattinfos.Add(ReconTraderAttributeRow(plotpos, plotrow).begin().Key(), ReconTraderAttributeRow(plotpos, plotrow).begin().Value());	break;
				case EDataCategory::DC_AIInfo:				 plottraderinfos.Add(ReconTraderInfoRow(plotpos, plotrow)[0]);																					break;
				case EDataCategory::DC_ForexDataInfo:		 plotforexvalue = plotrow.GetRowNameValuePairs().begin().Value();																					break;

				case EDataCategory::DC_DEFAULT:
				case EDataCategory::DC_MAX_ENTRY:
				default:																																														break;
			}

			plotpos++;

			if (plotforexvalue != 0 && plotresinfos.Num() >= resourceInfoLenght && plottraderattinfos.Num() >= attributeInfoLenght && plottraderinfos.Num() >= aiInfoLenght)
			{
				FPlotData newplot =
				{
					plotresinfos,
					plottraderattinfos,
					plottraderinfos,
					plotforexvalue
				};

				tablePlots.Add(newplot);

				plotresinfos.Empty();
				plottraderattinfos.Empty();
				plottraderinfos.Empty();
				plotforexvalue = 0;

				plotpos = 0;
			}

			if (tablePlots.Num() >= tableOverflowDebug)
				break;
		}

		allLoadedTables_Reconstructed.Add(allLoadedTables_Reconstructed.Num(), tablePlots);
	}
}

TMap<EResourceIdent, FResourceMarketPoolInfo> AActionDatabase::ReconResourceRow(int _plotPos, FCuratedDataRow _dataRow)
{
	TMap<EResourceIdent, FResourceMarketPoolInfo> plotresinfo;

	// should work if i dont change shit, because row 1 == EResourceIdent(1), exmpl.: so _plotPos 2 == RI_Water
	EResourceIdent resident = EResourceIdent::RI_DEFAULT;
	int resamount = 0;
	float unmodprice = 0;
	float modprice = 0;
	float delta = 0;
	float alpha = 0;
	float lambda = 0;
	float time = 0;

	for(TTuple<EDataName, float> column : _dataRow.GetRowNameValuePairs())
	{
		switch (column.Key)
		{
		case EDataName::DT_ResourceIdent:			   resident		= (EResourceIdent)(int)column.Value; break;
		case EDataName::DT_ResourceAmount:			   resamount	= (int)column.Value;				 break;
		case EDataName::DT_UnmodPrice:				   unmodprice	= column.Value;						 break;
		case EDataName::DT_ModPrice:				   modprice		= column.Value;						 break;
		case EDataName::DT_DemandDelta:				   delta		= column.Value;						 break;
		case EDataName::DT_DemandAlpha:				   alpha		= column.Value;						 break;
		case EDataName::DT_DemandLambda:			   lambda		= column.Value;						 break;
		case EDataName::DT_TimeSinceUpdate:			   time			= column.Value;						 break;

		case EDataName::DN_DEFAULT:					   
			default:																					 break;
		}
	}

	FResourceMarketPoolInfo poolinfo =
	{
		unmodprice,
		modprice,
		resamount,
		delta,
		alpha,
		lambda,
		time,
		resident
	};

	plotresinfo.Add(resident, poolinfo);

	return plotresinfo;
}

TMap<int, FAITraderAttributeInfo> AActionDatabase::ReconTraderAttributeRow(int _plotPos, FCuratedDataRow _dataRow)
{
	TMap<int, FAITraderAttributeInfo> plottraderattinfo;

	int aiid					 = 0;
	EResourceIdent prefsellident = EResourceIdent::RI_DEFAULT;
	int prefsellvalue			 = 0;
	EResourceIdent prefbuyident  = EResourceIdent::RI_DEFAULT;
	int prefbuyvalue			 = 0;
	EAIActions prefstateident	 = EAIActions::AA_DEFAULT;
	float prefstatevalue		 = 0;
	int amountmod				 = 0;
	float equilibriumamount		 = 0;
	int resourceGoalamount		 = 0;
	int currencyGoalamount		 = 0;

	for (TTuple<EDataName, float> column : _dataRow.GetRowNameValuePairs())
	{
		switch (column.Key)
		{
		case EDataName::DT_AIIndexAttribute:				aiid = column.Value;									 break;
		case EDataName::DT_PrefSellResource_Ident:			prefsellident = (EResourceIdent)(int)column.Value;		 break;
		case EDataName::DT_PrefSellResource_Value:			prefsellvalue = column.Value;							 break;
		case EDataName::DT_PrefBuyResource_Ident:			prefbuyident = (EResourceIdent)(int)column.Value;		 break;
		case EDataName::DT_PrefBuyResource_Value:			prefbuyvalue = column.Value;							 break;
		case EDataName::DT_PrefState_Ident:					prefstateident = (EAIActions)(int)column.Value;			 break;
		case EDataName::DT_PrefState_Value:					prefstatevalue = column.Value;							 break;
		case EDataName::DT_AmountModifier:					amountmod = column.Value;								 break;
		case EDataName::DT_EquilibriumAmount:				equilibriumamount = column.Value;						 break;
		case EDataName::DT_ResourceGoalAmount:				resourceGoalamount = column.Value;						 break;
		case EDataName::DT_CurrencyGoalAmount:				currencyGoalamount = column.Value;						 break;

		case EDataName::DN_DEFAULT:
		case EDataName::DN_MAX_ENTRY:
		default: 
			break;
		}
	}

	FAITraderAttributeInfo newinfo =
	{
		prefsellident,
		prefsellvalue,
		prefbuyident,
		prefbuyvalue,
		prefstateident,
		prefstatevalue,
		amountmod,
		equilibriumamount,
		resourceGoalamount,
		currencyGoalamount
	};


	plottraderattinfo.Add(aiid, newinfo);

	return plottraderattinfo;
}

TArray<FTraderInformations> AActionDatabase::ReconTraderInfoRow(int _plotPos, FCuratedDataRow _dataRow)
{
	TArray<FTraderInformations> plottraderinfo;

	int idx = 0;
	float currency = 0;
	TMap<EResourceIdent, int> pouch;
	bool bwonwithres = false;
	bool bwonwithcurr = false;
	bool bhaslost = false;

	TArray<EResourceIdent> residents;
	TArray<int> resvalues;

	for (TTuple<EDataName, float> column : _dataRow.GetRowNameValuePairs())
	{
		switch (column.Key)
		{
		case EDataName::DT_AIIndex:						idx = column.Value;														break;
		case EDataName::DT_Currency:					currency = column.Value;												break;
		case EDataName::DT_ResourcePouch_Ident_0:		residents.Add((EResourceIdent)(int)column.Value);						break;
		case EDataName::DT_ResourcePouch_Value_0:		resvalues.Add(column.Value);											break;
		case EDataName::DT_ResourcePouch_Ident_1:		residents.Add((EResourceIdent)(int)column.Value);						break;
		case EDataName::DT_ResourcePouch_Value_1:		resvalues.Add(column.Value);											break;
		case EDataName::DT_ResourcePouch_Ident_2:		residents.Add((EResourceIdent)(int)column.Value);						break;
		case EDataName::DT_ResourcePouch_Value_2:		resvalues.Add(column.Value);											break;
		case EDataName::DT_WonWithResources:			bwonwithres = UKismetMathLibrary::Conv_IntToBool((int)column.Value);	break;
		case EDataName::DT_WonWithCurrency:				bwonwithcurr = UKismetMathLibrary::Conv_IntToBool((int)column.Value);	break;
		case EDataName::DT_HasLost:						bhaslost = UKismetMathLibrary::Conv_IntToBool((int)column.Value);		break;


		case EDataName::DN_DEFAULT:
		case EDataName::DN_MAX_ENTRY:
		default: 
			break;
		}
	}

	int residx = 0;

	for(EResourceIdent res : residents)
	{
		pouch.Add(res, resvalues[residx]);

		residx++;
	}

	FTraderInformations traderinfo =
	{
		idx,
		currency,
		pouch,
		bwonwithres,
		bwonwithcurr,
		bhaslost
	};

	plottraderinfo.Add(traderinfo);

	return plottraderinfo;
}

FString AActionDatabase::GetSaveDesignation()
{
	FString designation = "";

	switch (dataToAverage) {
	case EDataName::DT_PlotIndex:					designation += "DT_PlotIndex";				break;
	case EDataName::DT_ResourceIdent:				designation += "DT_ResourceIdent";			break;
	case EDataName::DT_ResourceAmount:				designation += "DT_ResourceAmount";			break;
	case EDataName::DT_UnmodPrice:					designation += "DT_UnmodPrice";				break;
	case EDataName::DT_ModPrice:					designation += "DT_ModPrice";				break;
	case EDataName::DT_DemandDelta:					designation += "DT_DemandDelta";			break;
	case EDataName::DT_DemandAlpha:					designation += "DT_DemandAlpha";			break;
	case EDataName::DT_DemandLambda:				designation += "DT_DemandLambda";			break;
	case EDataName::DT_TimeSinceUpdate:				designation += "DT_TimeSinceUpdate";		break;
	case EDataName::DT_AIIndexAttribute:			designation += "DT_AIIndexAttribute";		break;
	case EDataName::DT_PrefSellResource_Ident:		designation += "DT_PrefSellResource_Ident";	break;
	case EDataName::DT_PrefSellResource_Value:		designation += "DT_PrefSellResource_Value";	break;
	case EDataName::DT_PrefBuyResource_Ident:		designation += "DT_PrefBuyResource_Ident";	break;
	case EDataName::DT_PrefBuyResource_Value:		designation += "DT_PrefBuyResource_Value";	break;
	case EDataName::DT_PrefState_Ident:				designation += "DT_PrefState_Ident";		break;
	case EDataName::DT_PrefState_Value:				designation += "DT_PrefState_Value";		break;
	case EDataName::DT_AmountModifier:				designation += "DT_AmountModifier";			break;
	case EDataName::DT_EquilibriumAmount:			designation += "DT_EquilibriumAmount";		break;
	case EDataName::DT_ResourceGoalAmount:			designation += "DT_ResourceGoalAmount";		break;
	case EDataName::DT_CurrencyGoalAmount:			designation += "DT_CurrencyGoalAmount";		break;
	case EDataName::DT_AIIndex:						designation += "DT_AIIndex";				break;
	case EDataName::DT_Currency:					designation += "DT_Currency";				break;
	case EDataName::DT_ResourcePouch_Ident_0:		designation += "DT_ResourcePouch_Ident_0";	break;
	case EDataName::DT_ResourcePouch_Value_0:		designation += "DT_ResourcePouch_Value_0";	break;
	case EDataName::DT_ResourcePouch_Ident_1:		designation += "DT_ResourcePouch_Ident_1";	break;
	case EDataName::DT_ResourcePouch_Value_1:		designation += "DT_ResourcePouch_Value_1";	break;
	case EDataName::DT_ResourcePouch_Ident_2:		designation += "DT_ResourcePouch_Ident_2";	break;
	case EDataName::DT_ResourcePouch_Value_2:		designation += "DT_ResourcePouch_Value_2";	break;
	case EDataName::DT_WonWithResources:			designation += "DT_WonWithResources";		break;
	case EDataName::DT_WonWithCurrency:				designation += "DT_WonWithCurrency";		break;
	case EDataName::DT_HasLost:						designation += "DT_HasLost";				break;
	case EDataName::DT_Weight:						designation += "DT_Weight";					break;


	case EDataName::DN_DEFAULT: 
	case EDataName::DN_MAX_ENTRY: 
	default:
		designation += "ERROR_DESIGNATION";
	}

	return designation;
}

int AActionDatabase::SampleDataSizePerPlot(FCuratedTable _firstTable)
{
	int datasize = 0;
	int idx = 0;

	for(FCuratedDataRow row : _firstTable.GetCuratedRowEntries())
	{
		for (TTuple<EDataName, float> column : row.GetRowNameValuePairs())
		{
			if(column.Key == dataToAverage)
			{
				datasize++;
				break;
			}
		}

		idx++;

		if (idx >= plotLenght)
			break;
	}

	return datasize;
}

TMap<EDataName, float> AActionDatabase::CreateNameValuePair(EDataCategory _category, TArray<float> _rowEntries)
{
	TMap<EDataName, float> pairs;

	// start value in EDataName
	int iterator = 0;

	// Corrosponds to the enum position
	iterator = DataRuleSet.GetLineSepByEnum().FindRef(_category);

	for(float column : _rowEntries)
	{
		EDataName name = EDataName(iterator);
		float value = column;

		pairs.Add(name, value);

		iterator++;
	}

	return pairs;
}

FString AActionDatabase::ConvertFloatToString(float _toConvert, int _lenght)
{
	FString returnstring;

	returnstring = UKismetStringLibrary::Conv_FloatToString(_toConvert);
	int stringlenght = returnstring.GetCharArray().Num();

	if(stringlenght > _lenght)
	{
		FString newstring = "";
		int idx = 0;

		bool indecimal = false;

		for(char character : returnstring)
		{
			newstring += character;

			if (character == '.' || character == ',')
				indecimal = true;
				
			if(!indecimal)
				continue;

			idx++;

			// not >= as to ignore , or .
			if (idx > _lenght)
				break;
		}

		returnstring = newstring;
	}

	return returnstring;
}

FString AActionDatabase::CreateMarketInfoString(FPlotData _plotData)
{
	FString marketinfostring = "";

	for (TTuple<EResourceIdent, FResourceMarketPoolInfo> resinfo : _plotData.GetResourceInfo())
	{
		FString ident = FString::FromInt((int)resinfo.Value.GetIdent());
		FString amount = FString::FromInt((int)resinfo.Value.GetAmount());

		FString unmodprice = ConvertFloatToString(resinfo.Value.GetUnmodifiedPrice(), maxFloatLenght);
		FString modprice = ConvertFloatToString(resinfo.Value.GetModifiedPrice(), maxFloatLenght);

		FString demanddelta = ConvertFloatToString(resinfo.Value.GetDemandDelta(), maxFloatLenght);
		FString demandalpha = ConvertFloatToString(resinfo.Value.GetDemandAlpha(), maxFloatLenght);

		FString demandlambda = ConvertFloatToString(resinfo.Value.GetDemandLambda(), maxFloatLenght);
		FString timesinceupdate = ConvertFloatToString(resinfo.Value.GetTimeSinceUpdate(), maxFloatLenght);

		marketinfostring += ident;
		marketinfostring += ',';

		marketinfostring += amount;
		marketinfostring += ',';

		marketinfostring += unmodprice;
		marketinfostring += ',';

		marketinfostring += modprice;
		marketinfostring += ',';

		marketinfostring += demanddelta;
		marketinfostring += ',';

		marketinfostring += demandalpha;
		marketinfostring += ',';

		marketinfostring += demandlambda;
		marketinfostring += ',';

		marketinfostring += timesinceupdate;
		marketinfostring += ',';

		marketinfostring += LINE_TERMINATOR;
	}

	return marketinfostring;
}

FString AActionDatabase::CreateAttributeInfoString(FPlotData _plotData)
{
	FString attributeinfostring = "";

	for (TTuple<int, FAITraderAttributeInfo> attributeinfo : _plotData.GetAllTraderAttributeInfos())
	{
		FString traderindex = FString::FromInt(attributeinfo.Key);

		FString prefsellresident = FString::FromInt((int)attributeinfo.Value.GetPrefSellResource().begin().Key());
		FString prefsellresvalue = FString::FromInt(attributeinfo.Value.GetPrefSellResource().begin().Value());

		FString prefbuyresident = FString::FromInt((int)attributeinfo.Value.GetPrefBuyResource().begin().Key());
		FString prefbuyresvalue = FString::FromInt(attributeinfo.Value.GetPrefBuyResource().begin().Value());

		FString prefstateident = FString::FromInt((int)attributeinfo.Value.GetPrefState().begin().Key());
		FString prefstatevalue = FString::FromInt(attributeinfo.Value.GetPrefState().begin().Value());

		FString amountmod = FString::FromInt(attributeinfo.Value.GetAmountModifier());
		FString eqlamount = ConvertFloatToString(attributeinfo.Value.GetEquilibriumAmount(), maxFloatLenght);

		FString resgoalamount = FString::FromInt(attributeinfo.Value.GetResourceGoalAmount());
		FString currgoalamount = FString::FromInt(attributeinfo.Value.GetCurrencyGoalAmount());

		attributeinfostring += traderindex;
		attributeinfostring += ',';

		attributeinfostring += prefsellresident;
		attributeinfostring += ',';

		attributeinfostring += prefsellresvalue;
		attributeinfostring += ',';

		attributeinfostring += prefbuyresident;
		attributeinfostring += ',';

		attributeinfostring += prefbuyresvalue;
		attributeinfostring += ',';

		attributeinfostring += prefstateident;
		attributeinfostring += ',';

		attributeinfostring += prefstatevalue;
		attributeinfostring += ',';

		attributeinfostring += amountmod;
		attributeinfostring += ',';

		attributeinfostring += eqlamount;
		attributeinfostring += ',';

		attributeinfostring += resgoalamount;
		attributeinfostring += ',';

		attributeinfostring += currgoalamount;
		attributeinfostring += ',';

		attributeinfostring += LINE_TERMINATOR;
	}

	return  attributeinfostring;
}

FString AActionDatabase::CreateAIInfoString(FPlotData _plotData)
{
	FString aiinfostring = "";

	for(FTraderInformations aiinfo  : _plotData.GetAllTraderInfos())
	{
		FString traderindex = FString::FromInt(aiinfo.GetIdent());
		FString currency = FString::FromInt(aiinfo.GetCurrency());

		TMap<FString, FString> identamountpair;

		for(TTuple<EResourceIdent, int> resource : aiinfo.GetPouch())
		{
			FString ident = FString::FromInt((int)(resource.Key));
			FString amount = FString::FromInt(resource.Value);

			identamountpair.Add(ident, amount);
		}

		FString wonwithresource = FString::FromInt((int)aiinfo.GetWonWithRes());
		FString wonwithcurrency = FString::FromInt((int)aiinfo.GetWonWithCurr());
		FString haslost = FString::FromInt((int)aiinfo.GetHasLost());

		aiinfostring += traderindex;
		aiinfostring += ',';
		aiinfostring += currency;
		aiinfostring += ',';

		for(TTuple<FString, FString> item :identamountpair)
		{
			aiinfostring += item.Key;
			aiinfostring += ',';
			aiinfostring += item.Value;
			aiinfostring += ',';
		}

		aiinfostring += wonwithresource;
		aiinfostring += ',';

		aiinfostring += wonwithcurrency;
		aiinfostring += ',';

		aiinfostring += haslost;
		aiinfostring += ',';

		aiinfostring += LINE_TERMINATOR;
	}

	return aiinfostring;
}

FString AActionDatabase::CreateForexInfo(FPlotData _plotData)
{
	FString forexinfostring = "";

	forexinfostring += ConvertFloatToString(_plotData.GetForexValue(), maxFloatLenght);
	forexinfostring += ",";
	forexinfostring += LINE_TERMINATOR;

	return forexinfostring;
}