
#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>
#include <limits>
#include <filesystem>

#include <chrono>
#include "date.h"
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include "FinancialAnalysisToolkit.h"
#include "JsonFunctions.h"
#include <sciplot/sciplot.hpp>
#include "PlottingFunctions.h"
#include "ReportingFunctions.h"


struct MetricSummaryDataSet{
  std::vector< std::string > ticker;
  std::vector< std::vector < date::sys_days > > date;  
  std::vector< std::vector< double > > metric;
  std::vector< std::vector< int > > metricRank;
  std::vector< double > metricRankSum;
  std::vector< size_t > rank;
  std::vector< size_t > sortedIndex;
  std::vector < std::vector < PlottingFunctions::SummaryStatistics > > summaryStatistics;
  std::vector < double > weight;
};

// Fun example to get the ranked index of an entry
// https://stackoverflow.com/questions/1577475/c-sorting-and-keeping-track-of-indexes

template <typename T>
std::vector<size_t> calcSortedVectorIndicies(const std::vector<T> &v, 
                                            bool sortAscending)
{
  std::vector<size_t> idx(v.size());
  std::iota(idx.begin(), idx.end(), 0);
  if (sortAscending)
  {
    std::stable_sort(idx.begin(), idx.end(),
                     [&v](size_t i1, size_t i2)
                     { return v[i1] < v[i2]; });
  }
  else
  {
    std::stable_sort(idx.begin(), idx.end(),
                     [&v](size_t i1, size_t i2)
                     { return v[i1] > v[i2]; });
  }
  return idx;
};

//==============================================================================
template <typename T>
std::vector< size_t > getRank(const std::vector<T> &indiciesOfSortedVector){

  std::vector<size_t> rank(indiciesOfSortedVector.size());
  for(size_t i = 0; i < indiciesOfSortedVector.size(); ++i){
    rank[indiciesOfSortedVector[i]] = i;
  }     
  return rank;                           

};

//==============================================================================

void rankMetricData(const nlohmann::ordered_json &marketReportConfig, 
                    MetricSummaryDataSet &metricDataSetUpd,             
                    bool verbose)
{

  if(verbose){
    std::cout << "----------------------------------------" << std::endl;
    std::cout <<  "Ranking metric data ..." <<std::endl;
    std::cout << "----------------------------------------" << std::endl;
  }

  //Evaluate the ranking of the individual metrics
  size_t indexMetric=0;

  metricDataSetUpd.rank.resize(metricDataSetUpd.metric.size());
  metricDataSetUpd.metricRankSum.resize(metricDataSetUpd.metric.size());
  metricDataSetUpd.sortedIndex.resize(metricDataSetUpd.metric.size());

  metricDataSetUpd.metricRank.resize(metricDataSetUpd.metric.size());
  for(size_t i=0; i<metricDataSetUpd.metric.size();++i){
    metricDataSetUpd.metricRank[i].resize(metricDataSetUpd.metric[i].size());
    metricDataSetUpd.rank[i]         =0;
    metricDataSetUpd.metricRankSum[i]=0;
    metricDataSetUpd.sortedIndex[i]  =0;
  }

  for( auto const &rankingItem : marketReportConfig["ranking"].items()){
    //Retrieve the metric data
    std::string metricName(rankingItem.key());

    double lowerBound = 
      JsonFunctions::getJsonFloat(rankingItem.value()["lowerBound"]);

    double upperBound = 
      JsonFunctions::getJsonFloat(rankingItem.value()["upperBound"]);

    std::string direction;
    JsonFunctions::getJsonString(rankingItem.value()["direction"],direction);


    int typeOfMeasure = -1;

    if(!rankingItem.value()["measure"].is_null()){
      std::string typeOfMeasureName;
      JsonFunctions::getJsonString(rankingItem.value()["measure"],typeOfMeasureName);

      if(typeOfMeasureName == "recent"){
        typeOfMeasure = -1;
      }else if(typeOfMeasureName == "p05"){
        typeOfMeasure = 0;
      }else if(typeOfMeasureName == "p25"){
        typeOfMeasure = 1;
      }else if(typeOfMeasureName == "p50"){
        typeOfMeasure = 2;
      }else if(typeOfMeasureName == "p75"){
        typeOfMeasure = 3;
      }else if(typeOfMeasureName == "p95"){
        typeOfMeasure = 4;
      }else{
        std::cout << "Error: value field must be recent,p05,p25,p50,p75, or p95 "
                  << "instead this was entered: " << typeOfMeasureName;
        std::abort();                
      }
    }


    bool sortAscending=false;
    if(direction.compare("smallestIsBest")==0){
      sortAscending=true;
    }else if(direction.compare("biggestIsBest")==0){
      sortAscending=false;
    }else{
      std::cout << "Error: ranking metric " 
                << metricName
                << " at index " << indexMetric
                << " has a sort direction of "
                << " but this field can only be smallestIsBest/biggestIsBest"
                << std::endl;
      std::abort();              
    }

    std::vector< double > column;
    for(size_t indexRow=0; 
      indexRow < metricDataSetUpd.metric.size(); ++indexRow ){
      
      double value = 0;
      
      switch(typeOfMeasure){
        case -1:
        {
          value=metricDataSetUpd.metric[indexRow][indexMetric];
        }
        break;
        case 0:
        {
          value = metricDataSetUpd
                  .summaryStatistics[indexRow][indexMetric].percentiles[0];
        }
        break;
        case 1:
        {
          value = metricDataSetUpd
                  .summaryStatistics[indexRow][indexMetric].percentiles[1];
        }
        break;
        case 2:
        {
          value = metricDataSetUpd
                  .summaryStatistics[indexRow][indexMetric].percentiles[2];
        }
        break;
        case 3:
        {
          value = metricDataSetUpd
                  .summaryStatistics[indexRow][indexMetric].percentiles[3];
        }
        break;
        case 4:
        {
          value = metricDataSetUpd
                  .summaryStatistics[indexRow][indexMetric].percentiles[4];
        }
        break;
        default:{
          std::cout << "Error: value field must be "
                    << "recent,p05,p25,p50,p75, or p95."
                    << std::endl;
          std::abort();  
        };
      }

      
      if(!std::isnan(lowerBound)){
        if(value < lowerBound){
          value = std::nan("1");
        }
      }
      if(!std::isnan(upperBound)){
        if(value > upperBound){
          value = std::nan("1");
        }
      }
      if(std::isnan(value)){
        if(sortAscending){
          if(!std::isnan(upperBound)){
            value = upperBound;
          }else{
            value = std::numeric_limits<double>::max();
          }
        }else{
          if(!std::isnan(lowerBound)){
            value = lowerBound;
          }else{
            value = -std::numeric_limits<double>::max();
          }
        }
      }
      column.push_back(value);
    }

    std::vector< size_t > indicesOfSortedVector = 
      calcSortedVectorIndicies(column, sortAscending);

    std::vector< size_t > columnRank = getRank(indicesOfSortedVector);

    double weight = 
      JsonFunctions::getJsonFloat(rankingItem.value()["weight"]);

    for(size_t indexRow=0; indexRow < column.size(); ++indexRow){
      metricDataSetUpd.metricRank[indexRow][indexMetric] = columnRank[indexRow];
      double value = 
        static_cast<double>(metricDataSetUpd.metricRank[indexRow][indexMetric]);
      value *= weight;
      metricDataSetUpd.metricRankSum[indexRow] += value;
    }
    ++indexMetric;

  }

  metricDataSetUpd.sortedIndex = 
    calcSortedVectorIndicies(metricDataSetUpd.metricRankSum, true);
  metricDataSetUpd.rank = getRank(metricDataSetUpd.sortedIndex);    


  bool here=true;

};

//==============================================================================

bool appendMetricData(const std::string &tickerFileName,  
                  const std::string &fundamentalFolder,                 
                  const std::string &historicalFolder,
                  const std::string &calculateDataFolder,
                  const nlohmann::ordered_json &marketReportConfig, 
                  const date::year_month_day &targetDate,
                  int maxTargetDateErrorInDays,
                  MetricSummaryDataSet &metricDataSetUpd,             
                  bool verbose){


  bool inputsAreValid=true;

  bool rankingFieldExists = marketReportConfig.contains("ranking");

  bool weightFieldExists = marketReportConfig.contains("market_weighting");
  

  if(rankingFieldExists){

    std::vector< double > metricVector;
    std::vector< date::sys_days > dateVector;
    std::vector< PlottingFunctions::SummaryStatistics> percentileVector;
    double weight=1.0;

    nlohmann::ordered_json targetJsonTable;
    bool loadedCalculateData = false;

    for( auto const &rankingItem : marketReportConfig["ranking"].items()){
      //Retrieve the metric data
      std::string metricName(rankingItem.key());

      std::string folder; 
      JsonFunctions::getJsonString(rankingItem.value()["folder"],folder);

      std::vector< std::string > fieldVector;
      std::string fieldName("");
      for( auto const &fieldEntry : rankingItem.value()["field"]){
        JsonFunctions::getJsonString(fieldEntry,fieldName);
        fieldVector.push_back(fieldName);
      }      

      // Fetch the target data file
      std::string fullPathFileName("");      
      if(folder == "calculateData"){
        fullPathFileName = calculateDataFolder;                      
      }else{
        std::cerr << "Error: in ranking " << metricName 
                  << " the folder field is " << folder 
                  << " but should calculateData" 
                  << std::endl;
        std::abort();                  
      }

      fullPathFileName.append(tickerFileName);      

      //Only load this file once.
      if(!loadedCalculateData){
        loadedCalculateData = 
          JsonFunctions::loadJsonFile(fullPathFileName,
                                      targetJsonTable,
                                      verbose); 
      }                                    

      // Read in all of the data for the metric.
      // Each of the different file types have different formats and so each 
      // needs its own function to load the data

      if(loadedCalculateData){


        //Extract the metric data closest to the target date
        std::string closestDate;
        date::sys_days closestDay;
        double targetMetricValue = 0.;

        int smallestDayError = 
          JsonFunctions::findClosestDate( targetJsonTable,
                                          targetDate,
                                          "%Y-%m-%d",
                                          closestDate,
                                          closestDay);

        double metricValue = 
          JsonFunctions::getJsonFloat(targetJsonTable[closestDate],fieldVector);
        bool metricValueValid = JsonFunctions::isJsonFloatValid(metricValue);

        if(metricValueValid){
          targetMetricValue = metricValue;         
        }

        //Build the historical list
        std::vector< double > metricData;
        for(auto &metricItem : targetJsonTable.items()){
          std::string itemDate(metricItem.key());

          double metricValue = 
            JsonFunctions::getJsonFloat(targetJsonTable[itemDate],fieldVector);
          bool metricValueValid = JsonFunctions::isJsonFloatValid(metricValue);

          if(metricValueValid){
            metricData.push_back(metricValue);          
          }
        }

        if(weightFieldExists && closestDate.length() > 0){
          std::string folderWeighting; 
          JsonFunctions::getJsonString( 
            marketReportConfig["market_weighting"]["folder"],folderWeighting);

          if( (folderWeighting.compare("calculateData") != 0) ){
            std::cout << "Error: market_weighting:folder in the "
                      << "market configuration  file should be calculateData "
                      << " but is instead " << folderWeighting
                      << std::endl;
          }

          std::vector< std::string > fieldAddress;
          for(auto &el : marketReportConfig["market_weighting"]["field"]){
            fieldAddress.push_back(el);
          }

          weight = JsonFunctions::getJsonFloat(
                        targetJsonTable[closestDate],fieldAddress);

        }

        sciplot::Vec metricDataVec(metricData.size());
        for(size_t i=0; i< metricData.size(); ++i){
          metricDataVec[i] = metricData[i];
        }
                                
        PlottingFunctions::SummaryStatistics percentileSummary;
        PlottingFunctions::extractSummaryStatistics(metricDataVec,
                                                    percentileSummary); 
        percentileSummary.current = targetMetricValue;                                                    

        if(smallestDayError <= maxTargetDateErrorInDays){
          metricVector.push_back(targetMetricValue);
        }else{
          metricVector.push_back(
              std::numeric_limits< double >::signaling_NaN());
        }
        dateVector.push_back(closestDay);
        percentileVector.push_back(percentileSummary);
      }else{
        inputsAreValid=false;
      }
    }
    if(inputsAreValid){
      metricDataSetUpd.ticker.push_back(tickerFileName);
      metricDataSetUpd.date.push_back(dateVector);
      metricDataSetUpd.metric.push_back(metricVector);
      metricDataSetUpd.summaryStatistics.push_back(percentileVector);
      metricDataSetUpd.weight.push_back(weight);
    }
  }


  return inputsAreValid;
}

//==============================================================================

bool applyFilter(const std::string &tickerFileName,  
                 const std::string &fundamentalFolder,
                 const std::string &historicalFolder,
                 const std::string &calculateDataFolder,
                 const std::string &tickerReportFolder,
                 const nlohmann::ordered_json &marketReportConfig,
                 const date::year_month_day &targetDate,
                 int maxTargetDateErrorInDays,                 
                 bool replaceNansWithMissingData,
                 bool verbose){

  if(tickerFileName.compare("MOZN.SW.json")==0){
    bool here=true;
  }

  bool valueFilter = true;

  bool filterFieldExists = marketReportConfig.contains("filter");

  if(filterFieldExists){

    for( auto const &el : marketReportConfig["filter"].items() ){
      //
      // Retreive the filter data
      //
      std::string filterName(el.key());

      std::string folder; 
      JsonFunctions::getJsonString(el.value()["folder"],folder);

      std::vector< std::string > fieldVector;
      std::string fieldName("");
      for( auto const &fieldEntry : el.value()["field"]){
        JsonFunctions::getJsonString(fieldEntry,fieldName);
        fieldVector.push_back(fieldName);
      }

      std::string conditionName("");
      JsonFunctions::getJsonString(el.value()["condition"],conditionName);

      std::string operatorName("");
      JsonFunctions::getJsonString(el.value()["operator"],operatorName);

      std::string valueType("");
      JsonFunctions::getJsonString(el.value()["valueType"],valueType);


      std::vector< std::string > valueStringVector;
      std::string valueStringName;
      if(valueType == "string"){
        for( auto const &fieldEntry : el.value()["values"]){
          JsonFunctions::getJsonString(fieldEntry,valueStringName);
          valueStringVector.push_back(valueStringName);
        }  
      }

      std::vector< double > valueFloatVector;
      double valueFloatName;
      if(valueType == "float"){
        for( auto const &fieldEntry : el.value()["values"]){
          valueFloatName = JsonFunctions::getJsonFloat(fieldEntry);
          valueFloatVector.push_back(valueFloatName);
        }  
      }

      //
      // Fetch the target data and apply the filter
      //
      std::string fullPathFileName(""); 
      bool jsonTableItemizedByDate=false;     
      if(folder == "fundamentalData"){
        fullPathFileName = fundamentalFolder;
        jsonTableItemizedByDate=false;                
      }else if(folder == "historicalData"){
        fullPathFileName = historicalFolder;                        
        jsonTableItemizedByDate=true;                
      }else if(folder == "calculateData"){
        fullPathFileName = calculateDataFolder;       
        jsonTableItemizedByDate=true;                
      }else{
        std::cerr << "Error: in filter " << filterName 
                  << " the folder field is " << folder 
                  << " but should be fundamentalData, historicalData,"
                  << " or calculateData" 
                  << std::endl;
        std::abort();                  
      }

      fullPathFileName.append(tickerFileName);

      nlohmann::ordered_json targetJsonTable;
      bool loadedConfiguration = 
        JsonFunctions::loadJsonFile(fullPathFileName,
                                    targetJsonTable,
                                    verbose); 

      if(!loadedConfiguration){
        valueFilter = false;
        break;
      }

      //Go fetch the most recent date and prepend it to the field vector
      if(jsonTableItemizedByDate){
        std::string closestDate;
        date::sys_days closestDay;
        int smallestDayError = 
          JsonFunctions::findClosestDate( targetJsonTable,
                                          targetDate,
                                          "%Y-%m-%d",
                                          closestDate,
                                          closestDay);

        if(closestDate.length()==0){
          valueFilter=false;
          break;
        }else{
          fieldVector.insert(fieldVector.begin(),closestDate);
        }                                         
        
      }

      std::string valueString;
      double valueFloat;
      std::vector < bool > valueBoolVector;

      if(valueType == "string"){
        JsonFunctions::getJsonString(targetJsonTable, fieldVector, valueString);

        //Evaluate all of the individual values against the target
        for(size_t i = 0; i <  valueStringVector.size(); ++i){
          if(conditionName == "=="){
            bool equalityTest = (valueString == valueStringVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == "!="){
            bool equalityTest = (valueString != valueStringVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else{
            std::cerr << "Error: in filter " << filterName 
                  << " the condition field should be " 
                  << " == or != for a string"
                  << " but is instead " << conditionName  
                  << std::endl;
            std::abort();   
          }        
        }

      }else if(valueType == "float"){
        valueFloat = JsonFunctions::getJsonFloat(targetJsonTable, fieldVector,
                                                 replaceNansWithMissingData);

        //Evaluate all of the individual values against the target
        for(size_t i = 0; i <  valueFloatVector.size(); ++i){
          if(conditionName == "=="){
            bool equalityTest = (valueFloat == valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == "!="){
            bool equalityTest = (valueFloat != valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == ">"){
            bool equalityTest = (valueFloat > valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == "<"){
            bool equalityTest = (valueFloat < valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == ">="){
            bool equalityTest = (valueFloat >= valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == "<="){
            bool equalityTest = (valueFloat <= valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else{
            std::cerr << "Error: in filter " << filterName 
                  << " the condition field should be one of" 
                  << " (==, !=, >, <, >=, <=) for a float"
                  << " but is instead " << conditionName  
                  << std::endl;
            std::abort();   
          }        
        }


      }else{
        std::cerr << "Error: in filter " << filterName 
                  << " valueType should be string or float but is instead" 
                  << valueType 
                  << std::endl;        
      }


      //Apply the operator between all of the values to yield the final 
      //filter value

      bool valueFilterSingle = valueBoolVector[0];      
      if(valueBoolVector.size()>1){

        for(size_t i = 1; i < valueBoolVector.size(); ++i){
          if(operatorName == "||"){
            valueFilterSingle = (valueFilterSingle || valueBoolVector[i]);
          }else if( operatorName == "&&"){
            valueFilterSingle = (valueFilterSingle && valueBoolVector[i]);
          }else{
            std::cerr << "Error: in filter " << filterName 
                  << " the operator field should be " 
                  << " || or && for a string"
                  << " but is instead " << operatorName  
                  << std::endl;
            std::abort();             
          }
        }
      }else{
       valueFilterSingle = valueBoolVector[0];
      }
      valueFilter = (valueFilter && valueFilterSingle); 
    }

  }

  //Check to see if the ticker report file exists
  if(valueFilter){
    std::string tickerReportPath    = tickerReportFolder;
    std::string tickerFolder        = tickerFileName;
    std::string tickerLaTeXFileName = tickerFileName;

    ReportingFunctions::sanitizeFolderName(tickerFolder,true);
    ReportingFunctions::sanitizeFolderName(tickerLaTeXFileName,true);
    tickerLaTeXFileName.append(".tex");

    std::filesystem::path tickerLaTeXReportPath=tickerReportFolder;
    tickerLaTeXReportPath /= tickerFolder;
    tickerLaTeXReportPath /= tickerLaTeXFileName;

    if( !std::filesystem::exists(tickerLaTeXReportPath)  ){
      valueFilter = false;
    }
  }  

  return valueFilter;
};

//==============================================================================
void plotReportData(size_t indexTickerStart,
                    size_t indexTickerEnd,
                    const nlohmann::ordered_json &marketReportConfig, 
                    const MetricSummaryDataSet &metricDataSet,
                    const PlottingFunctions::PlotSettings &settings,
                    const std::string &marketReportFolder,
                    const std::string &summaryPlotFileName,
                    bool verbose)
{

  if(verbose){

    std::cout << "----------------------------------------" << std::endl;    
    std::cout << "Generating summary plot: " << summaryPlotFileName<< std::endl;
    std::cout << "----------------------------------------" << std::endl;              
  }

  bool rankingFieldExists = marketReportConfig.contains("ranking");

  double canvasWidth=0.;
  double canvasHeight=0.;


  if(rankingFieldExists){

    size_t numberOfRankingItems = marketReportConfig["ranking"].size();
    std::vector< std::vector< sciplot::PlotVariant > > arrayOfPlotVariant;


    std::vector< std::vector< sciplot::Plot2D >> arrayOfPlot2D;
    arrayOfPlot2D.resize(numberOfRankingItems);
    for(size_t i=0; i<numberOfRankingItems; ++i){
      arrayOfPlot2D[i].resize(1);
    }

    size_t indexMetric=0;

    for( auto const &rankingItem : marketReportConfig["ranking"].items()){
      
      PlottingFunctions::PlotSettings subplotSettings = settings;
      subplotSettings.lineWidth = 0.5;

      PlottingFunctions::SummaryStatistics marketSummary;   
      marketSummary.percentiles.resize(
          PlottingFunctions::PercentileIndices::NUM_PERCENTILES,0.);

      double marketWeight = 0;

      double weight = 
        JsonFunctions::getJsonFloat(rankingItem.value()["weight"]);

      double lowerBoundPlot = 
        JsonFunctions::getJsonFloat(
          rankingItem.value()["plotSettings"]["lowerBound"]);

      double upperBoundPlot = 
        JsonFunctions::getJsonFloat(
          rankingItem.value()["plotSettings"]["upperBound"]);

      bool useLogarithmicScaling=
        JsonFunctions::getJsonBool(
          rankingItem.value()["plotSettings"]["logarithmic"]);
      subplotSettings.logScale=useLogarithmicScaling;

      double width = 
        JsonFunctions::getJsonFloat(
          rankingItem.value()["plotSettings"]["width"]);

      double height = 
        JsonFunctions::getJsonFloat(
          rankingItem.value()["plotSettings"]["height"]);

      subplotSettings.plotWidthInPoints=
        PlottingFunctions::convertCentimetersToPoints(width);
      subplotSettings.plotHeightInPoints=
        PlottingFunctions::convertCentimetersToPoints(height);

      canvasWidth=subplotSettings.plotWidthInPoints;
      canvasHeight+=subplotSettings.plotHeightInPoints;

      std::string direction;
      JsonFunctions::getJsonString(rankingItem.value()["direction"],direction);
      bool smallestIsBest=false;
      bool biggestIsBest=false;
      if(direction.compare("smallestIsBest")==0){
        smallestIsBest=true;
      }
      if(direction.compare("biggestIsBest")==0){
        biggestIsBest=true;
      }

      double threshold = 
        JsonFunctions::getJsonFloat(
          rankingItem.value()["plotSettings"]["threshold"]);
      
      sciplot::Vec xThreshold(2);
      sciplot::Vec yThreshold(2);
      xThreshold[0] = static_cast<double>(indexTickerStart)+1.0;
      xThreshold[1] = static_cast<double>(indexTickerEnd)+1.0;
      yThreshold[0] = threshold;
      yThreshold[1] = threshold;

      arrayOfPlot2D[indexMetric][0].drawCurve(xThreshold,yThreshold)
        .lineColor("gray")
        .lineWidth(settings.lineWidth*0.5)
        .labelNone();

      size_t tickerCount=0;
      for( size_t i=indexTickerStart; i< indexTickerEnd; ++i){

        size_t indexSorted = metricDataSet.sortedIndex[i];

        double xMid = static_cast<double>(i)+1.0;
        double xWidth = 0.4;

        std::string currentColor("black");
        std::string boxColor("light-gray");
        int currentLineType = 0;

        if(metricDataSet.summaryStatistics[indexSorted][indexMetric].percentiles.size()>0){
          if(smallestIsBest && 
            metricDataSet.summaryStatistics[indexSorted][indexMetric].current 
            <= threshold){
            currentLineType = 1;
          }
          if(biggestIsBest && 
            metricDataSet.summaryStatistics[indexSorted][indexMetric].current 
              >= threshold){    
            currentLineType = 1;      
          }

          PlottingFunctions::drawBoxAndWhisker(
            arrayOfPlot2D[indexMetric][0],
            xMid,
            xWidth,
            metricDataSet.summaryStatistics[indexSorted][indexMetric],
            boxColor.c_str(),
            currentColor.c_str(),
            currentLineType,
            subplotSettings,
            verbose);

          for(size_t j=0; j < marketSummary.percentiles.size();++j){
            marketSummary.percentiles[j] += 
              metricDataSet.summaryStatistics[indexSorted][indexMetric].percentiles[j]
              *metricDataSet.weight[indexSorted];
          }

          marketSummary.current +=
            metricDataSet.summaryStatistics[indexSorted][indexMetric].current
            *metricDataSet.weight[indexSorted];

          marketSummary.min +=  
            metricDataSet.summaryStatistics[indexSorted][indexMetric].min
            *metricDataSet.weight[indexSorted];

          marketSummary.max +=  
            metricDataSet.summaryStatistics[indexSorted][indexMetric].max
            *metricDataSet.weight[indexSorted];

          marketWeight += metricDataSet.weight[indexSorted];

          ++tickerCount;
        }
      }

      //Plot the market summary
      for(size_t j=0; j < marketSummary.percentiles.size();++j){
        marketSummary.percentiles[j] = 
          marketSummary.percentiles[j] / marketWeight;
      }
      marketSummary.min     = marketSummary.min / marketWeight; 
      marketSummary.max     = marketSummary.max / marketWeight; 
      marketSummary.current = marketSummary.current / marketWeight; 


      std::string currentColor("black");
      std::string boxColor("chartreuse");
      int currentLineType = 0;

      if(smallestIsBest && marketSummary.current <= threshold){
          currentLineType=1;
      }
      if(biggestIsBest && marketSummary.current  >= threshold){
          currentLineType=1;
      }

      double xMid = static_cast<double>(indexTickerEnd)+1.0;
      double xWidth = 0.4;
      PlottingFunctions::drawBoxAndWhisker(
        arrayOfPlot2D[indexMetric][0],
        xMid,
        xWidth,
        marketSummary,
        boxColor.c_str(),
        currentColor.c_str(),
        currentLineType,
        subplotSettings,
        verbose);
      
      //Add the labels
      std::string xAxisLabel("Ranking");

      std::string yAxisLabel(rankingItem.key());
      yAxisLabel.append(" (");
      std::stringstream stream;
      stream << std::fixed << std::setprecision(2) << weight;
      yAxisLabel.append( stream.str() );
      yAxisLabel.append(")");


      PlottingFunctions::configurePlot(
        arrayOfPlot2D[indexMetric][0],
        xAxisLabel,
        yAxisLabel,
        subplotSettings);

      if(!std::isnan(lowerBoundPlot) && !std::isnan(upperBoundPlot) ){
        arrayOfPlot2D[indexMetric][0].yrange(lowerBoundPlot,upperBoundPlot);
      }

      arrayOfPlot2D[indexMetric][0].xrange(
          static_cast<double>(indexTickerStart)-xWidth+1.0,
          static_cast<double>(indexTickerEnd)+xWidth+1.0);

      arrayOfPlot2D[indexMetric][0].legend().hide();
      
      std::vector< sciplot::PlotVariant > rowOfPlotVariant;      
      rowOfPlotVariant.push_back(arrayOfPlot2D[indexMetric][0]);
      arrayOfPlotVariant.push_back(rowOfPlotVariant);

      ++indexMetric;
    } 

    sciplot::Figure figSummary(arrayOfPlotVariant);
    figSummary.title("Summary");
    sciplot::Canvas canvas = {{figSummary}};

    canvas.size(canvasWidth, canvasHeight) ;    

    // Save the figure to a PDF file
    std::string outputFileName = marketReportFolder;
    outputFileName.append(summaryPlotFileName);
    canvas.save(outputFileName);     
  }

};

//==============================================================================
void generateLaTeXReport(
                    size_t indexTickerStart,
                    size_t indexTickerEnd,
                    const nlohmann::ordered_json &marketReportConfig, 
                    const MetricSummaryDataSet &metricDataSet,
                    const std::vector< std::string > &marketSummaryPlots,
                    const std::string &tickerReportFolder,
                    const std::string &marketReportFolder,
                    const std::string &marketReportFileName,
                    bool verbose){

  if(verbose){
    std::cout << "----------------------------------------" << std::endl;    
    std::cout << "Generating LaTeX report: " << marketReportFileName
              << std::endl;
    std::cout << "----------------------------------------" << std::endl;              
  }


  std::string outputReportPath(marketReportFolder);
  outputReportPath.append(marketReportFileName);

  std::string latexReportPath(outputReportPath);
  std::ofstream latexReport;

  try{
    latexReport.open(latexReportPath);
  }catch(std::ofstream::failure &ofstreamErr){
    std::cerr << std::endl << std::endl 
              << "Error: an exception was thrown trying to open " 
              << latexReportPath              
              << " :"
              << latexReportPath << std::endl << std::endl
              << ofstreamErr.what()
              << std::endl;    
    std::abort();
  }  

  // Write the opening to the latex file
  latexReport << "\\documentclass[11pt,onecolumn,a4paper]{article}" 
              << std::endl;
  latexReport << "\\usepackage[hmargin={1.35cm,1.35cm},vmargin={2.0cm,3.0cm},"
                    "footskip=0.75cm,headsep=0.25cm]{geometry}"<<std::endl;
  latexReport << "\\usepackage{graphicx,caption}"<<std::endl;
  latexReport << "\\usepackage{times}"<<std::endl;
  latexReport << "\\usepackage{graphicx}"<<std::endl;
  latexReport << "\\usepackage{hyperref}"<<std::endl;
  latexReport << "\\usepackage{multicol}"<<std::endl;
  latexReport << "\\usepackage[usenames,dvipsnames,table]{xcolor}"<<std::endl;

  // Add the opening figure
  std::string reportTitle;
  JsonFunctions::getJsonString(marketReportConfig["report"]["title"],reportTitle);
  latexReport << "\\title{" << reportTitle <<"}" << std::endl;

  latexReport << std::endl << std::endl;
  latexReport << "\\begin{document}" << std::endl << std::endl;
  latexReport << "\\maketitle" << std::endl;
  latexReport << "\\today" << std::endl;

  for(auto const &figName : marketSummaryPlots){
    latexReport << "\\begin{figure}[h]" << std::endl;
    latexReport << "  \\begin{center}" << std::endl;
    latexReport << "    \\includegraphics{"
                << marketReportFolder << figName
                << "}" << std::endl;
    latexReport << " \\end{center}" << std::endl;
    latexReport << "\\end{figure}"<< std::endl;
    latexReport << std::endl;    
  }

  // Add the list of companies
  latexReport << "\\begin{multicols}{3}" << std::endl;
  latexReport << "\\begin{enumerate}" << std::endl;
  latexReport << "\\setcounter{enumi}{" << indexTickerStart <<"}" << std::endl;  
  latexReport << "\\itemsep0pt" << std::endl;

  double totalMarketWeight=0.;
  for(size_t i = 0; i < metricDataSet.ticker.size(); ++i){
    totalMarketWeight += metricDataSet.weight[i];
  }  

  for(size_t i = indexTickerStart; i < indexTickerEnd; ++i){
    size_t j = metricDataSet.sortedIndex[i];

    double weight  = metricDataSet.weight[j];
    std::stringstream stream;
    stream << std::fixed << std::setprecision(3) 
           << (weight/totalMarketWeight)*100.0
           << "\\%";

    std::string tickerString(metricDataSet.ticker[j]);
    ReportingFunctions::sanitizeStringForLaTeX(tickerString,true);

    std::string tickerLabel(metricDataSet.ticker[j]);
    ReportingFunctions::sanitizeLabelForLaTeX(tickerLabel,true);

    latexReport << "\\item " << "\\ref{" << tickerLabel << "} "
                <<  tickerString << "---" << metricDataSet.metricRankSum[j] 
                << std::endl;
  }
  latexReport << "\\end{enumerate}" << std::endl;
  latexReport << "\\end{multicols}" << std::endl;
  latexReport << std::endl;
  
  // Append the ticker reports in oder
  for(size_t i = indexTickerStart; i < indexTickerEnd; ++i){
    size_t j = metricDataSet.sortedIndex[i];

    std::string tickerString(metricDataSet.ticker[j]);
    std::string tickerLabel(metricDataSet.ticker[j]);
    std::string tickerFile(metricDataSet.ticker[j]);

    ReportingFunctions::sanitizeStringForLaTeX(tickerString,true);
    ReportingFunctions::sanitizeLabelForLaTeX(tickerLabel,true);
    ReportingFunctions::sanitizeFolderName(tickerFile,true);

    std::filesystem::path tickerReportPath=tickerReportFolder;
    tickerReportPath /= tickerFile;
    tickerReportPath /= tickerFile;
    tickerReportPath += ".tex";

    std::filesystem::path graphicsPath=tickerReportFolder;
    graphicsPath /= tickerFile;



    latexReport << "\\break" << std::endl;
    latexReport << "\\newpage" << std::endl << std::endl;

    latexReport << "\\section{ " << tickerString << " }" <<std::endl; 
    latexReport << "\\label{" << tickerLabel << "}" << std::endl;
    latexReport << "\\graphicspath{{" << graphicsPath.string() << "}}" 
                << std::endl;
    latexReport << "\\input{"
                << tickerReportPath.string()
                << "}"
                << std::endl
                << std::endl;
  }
  //close the file

  latexReport << "\\end{document}" << std::endl;
  latexReport.close();

};

//==============================================================================
int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string calculateDataFolder;
  std::string fundamentalFolder;
  std::string historicalFolder;
  std::string marketReportConfigurationFilePath;  
  std::string tickerReportFolder;
  std::string marketReportFolder;  
  std::string dateOfTable;

  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will produce a market report in the form of "
    "text and tables about the companies that are best ranked."
    ,' ', "0.0");

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","exchange_code", 
      "The exchange code. For example: US",
      false,"","string");
    cmd.add(exchangeCodeInput);  

    TCLAP::ValueArg<std::string> tickerReportFolderOutput("t","ticker_report_folder_path", 
      "The path to the folder will contains the ticker reports.",
      true,"","string");
    cmd.add(tickerReportFolderOutput);

    TCLAP::ValueArg<std::string> marketReportFolderOutput("o","market_report_folder_path", 
      "The path to the folder will contains the market report.",
      true,"","string");
    cmd.add(marketReportFolderOutput);    

    TCLAP::ValueArg<std::string> calculateDataFolderInput("a","calculate_folder_path", 
      "The path to the folder contains the output "
      "produced by the calculate method",
      true,"","string");
    cmd.add(calculateDataFolderInput);

    TCLAP::ValueArg<std::string> fundamentalFolderInput("f",
      "fundamental_data_folder_path", 
      "The path to the folder that contains the fundamental data json files from "
      "https://eodhistoricaldata.com/ to analyze",
      true,"","string");
    cmd.add(fundamentalFolderInput);

    TCLAP::ValueArg<std::string> historicalFolderInput("p",
      "historical_data_folder_path", 
      "The path to the folder that contains the historical (price)"
      " data json files from https://eodhistoricaldata.com/ to analyze",
      true,"","string");
    cmd.add(historicalFolderInput);

    TCLAP::ValueArg<std::string> dateOfTableInput("d","date", 
      "Date used to produce the dicounted cash flow model detailed output.",
      false,"","string");
    cmd.add(dateOfTableInput);

    TCLAP::ValueArg<std::string> marketReportConfigurationFilePathInput("c",
      "market_report_configuration_file", 
      "The path to the json file that contains the names of the metrics "
      " to plot.",
      true,"","string");

    cmd.add(marketReportConfigurationFilePathInput);    

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    exchangeCode              = exchangeCodeInput.getValue();  
    marketReportConfigurationFilePath 
                              = marketReportConfigurationFilePathInput.getValue();      
    calculateDataFolder       = calculateDataFolderInput.getValue();
    fundamentalFolder         = fundamentalFolderInput.getValue();
    historicalFolder          = historicalFolderInput.getValue();    
    tickerReportFolder        = tickerReportFolderOutput.getValue();
    marketReportFolder        = marketReportFolderOutput.getValue();    
    dateOfTable               = dateOfTableInput.getValue();
    verbose                   = verboseInput.getValue();

    if(verbose){
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;


      if(dateOfTable.length()>0){
        std::cout << "  Date used to calculate tabular output" << std::endl;
        std::cout << "    " << dateOfTable << std::endl;
      }

      std::cout << "  Calculate Data Input Folder" << std::endl;
      std::cout << "    " << calculateDataFolder << std::endl;

      std::cout << "  Fundamental Data Folder" << std::endl;
      std::cout << "    " << fundamentalFolder << std::endl;

      std::cout << "  Historical Data Folder" << std::endl;
      std::cout << "    " << historicalFolder << std::endl;

      std::cout << "  Ticker Report Folder" << std::endl;
      std::cout << "    " << tickerReportFolder << std::endl;  

      std::cout << "  Market Report Configuration File" << std::endl;
      std::cout << "    " << marketReportConfigurationFilePath << std::endl;          

      std::cout << "  Market Report Folder" << std::endl;
      std::cout << "    " << marketReportFolder << std::endl;  

    }

  } catch (TCLAP::ArgException &e){ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }

  bool replaceNansWithMissingData = true;

  auto today = date::floor<date::days>(std::chrono::system_clock::now());
  date::year_month_day targetDate(today);
  int maxTargetDateErrorInDays = 365;

  //Load the report configuration file
  nlohmann::ordered_json marketReportConfig;
  bool loadedConfiguration = 
    JsonFunctions::loadJsonFile(marketReportConfigurationFilePath,
                                marketReportConfig,
                                verbose);
                                  

  if(!loadedConfiguration){
    std::cerr << "Error: cannot open " << marketReportConfigurationFilePath 
              << std::endl;
    
  }



  if(loadedConfiguration){
    std::vector< std::string > processedTickers;

    int totalFileCount = 0;
    int validFileCount = 0;
    std::string analysisExt = ".json";  



    //Go through every ticker file in the calculate folder:
    //1. Create a ticker folder in the reporting folder
    //2. Save to this ticker folder:
    //  a. The pdf of the ticker's plots
    //  b. The LaTeX file for the report (as an input)
    //  c. A wrapper LaTeX file to generate an individual report

    std::vector< std::string > filteredTickers;

    for (const auto & file 
          : std::filesystem::directory_iterator(calculateDataFolder)){  

      ++totalFileCount;
      bool validInput = true;

      std::string fileName   = file.path().filename();

      std::size_t fileExtPos = fileName.find(analysisExt);

      if(verbose){
        std::cout << totalFileCount << '\t' << fileName << std::endl;
      }
      
      if( fileExtPos == std::string::npos ){
        validInput=false;
        if(verbose){
          std::cout << '\t' 
                    << "Skipping " << fileName 
                    << " should end in .json but this does not:" << std::endl; 
        }
      }


      //Check to see if this ticker passes the filter
      bool tickerPassesFilter=false;
      if(validInput){
        tickerPassesFilter = applyFilter( fileName,  
                                  fundamentalFolder,
                                  historicalFolder,
                                  calculateDataFolder,
                                  tickerReportFolder,
                                  marketReportConfig,
                                  targetDate,
                                  maxTargetDateErrorInDays,                                  
                                  replaceNansWithMissingData,
                                  verbose);
      }    

      if(tickerPassesFilter){
        filteredTickers.push_back(fileName);
      }
      if(verbose){
        if(tickerPassesFilter){
          std::cout << '\t'
                    << "passed"
                    << std::endl;
        }else{
          std::cout << '\t'
                    << "filtered out"
                    << std::endl;
        }
      }

    }

    if(verbose){
      std::cout << '\n' << '\n'
                << filteredTickers.size() << '\t' 
                << "files passed the filter"
                << '\n' 
                << totalFileCount - filteredTickers.size() << '\t' 
                << "files removed"
                << std::endl;

      std::cout << '\n' << '\n'
                << "Tickers that passed the filter: " << std::endl;
      for(size_t i=0; i< filteredTickers.size(); ++i){
        std::cout << i << '\t' << filteredTickers[i] << std::endl;
      }                

    }

    //
    // Collect all of the metrics used for ranking
    //

    MetricSummaryDataSet metricDataSet;    
    if(filteredTickers.size() > 0){
      if(verbose){
        std::cout << "Appending metric data ..."  << std::endl << std::endl;
      }

      for (size_t i=0; i<filteredTickers.size(); ++i){

        if(verbose){
          std::cout << i << "." << '\t' << filteredTickers[i] << std::endl;
        }        

        bool appendedMetricData = 
            appendMetricData(filteredTickers[i],  
                        fundamentalFolder,                 
                        historicalFolder,
                        calculateDataFolder,
                        marketReportConfig,
                        targetDate,
                        maxTargetDateErrorInDays,
                        metricDataSet,                        
                        verbose);
        
        if(verbose && !appendedMetricData){
          std::cout << "Skipping: " 
                    << filteredTickers[i] 
                    << ": missing valid metric data."
                    << std::endl;
        }                        
      }

      rankMetricData(marketReportConfig,metricDataSet,verbose);

    }

    std::string marketReportConfigurationFileName =
      std::filesystem::path(marketReportConfigurationFilePath).filename();

    ReportingFunctions::sanitizeFolderName(marketReportConfigurationFileName);  

    int numberOfTickersPerReport = 50;
    int maximumNumberOfReports=1;
    int maximumNumberOfReportsDefault = 
      static_cast< int >(
        std::ceil(static_cast<double>(metricDataSet.ticker.size())
                /static_cast<double>(numberOfTickersPerReport))
      );

    if(marketReportConfig.contains("report")){
      if(marketReportConfig["report"].contains("number_of_tickers_per_report")){
        double tmp = JsonFunctions::getJsonFloat(
          marketReportConfig["report"]["number_of_tickers_per_report"],false);
        if(!std::isnan(tmp)){
          numberOfTickersPerReport = static_cast<int>(tmp);
        }
      }
      if(marketReportConfig["report"].contains("number_of_reports")){
        double tmp = JsonFunctions::getJsonFloat(
          marketReportConfig["report"]["number_of_reports"],false);
        if(!std::isnan(tmp)){
          maximumNumberOfReports = static_cast<int>(tmp);
        }        
      }      
    }

    maximumNumberOfReports = std::min(maximumNumberOfReports,
                                      maximumNumberOfReportsDefault);

    for(size_t indexReport=0; indexReport<maximumNumberOfReports;++indexReport){

      size_t indexStart = (numberOfTickersPerReport)*indexReport;
      size_t indexEnd   = std::min( indexStart+numberOfTickersPerReport,
                                    metricDataSet.ticker.size()       );  

      std::stringstream reportNumber;
      reportNumber << indexReport;
      std::string reportNumberStr(reportNumber.str());

      while(reportNumberStr.length()<3){
        std::string tmp("0");
        reportNumberStr = tmp.append(reportNumberStr);
      }

      std::string summaryPlotFileName("summary_");
      summaryPlotFileName.append(marketReportConfigurationFileName);
      summaryPlotFileName.append("_");
      summaryPlotFileName.append(reportNumberStr);
      summaryPlotFileName.append(".pdf");
      
      PlottingFunctions::PlotSettings settings;

      std::vector< std::string > marketSummaryPlots;
      marketSummaryPlots.push_back(summaryPlotFileName);

      plotReportData(indexStart,
                    indexEnd,
                    marketReportConfig, 
                    metricDataSet,
                    settings,
                    marketReportFolder,
                    summaryPlotFileName,
                    verbose);      

    std::string marketReportFileName("report_");
    marketReportFileName.append(marketReportConfigurationFileName);
    marketReportFileName.append("_");
    marketReportFileName.append(reportNumberStr);    
    marketReportFileName.append(".tex");

    generateLaTeXReport(indexStart,
                        indexEnd,
                        marketReportConfig, 
                        metricDataSet,
                        marketSummaryPlots,
                        tickerReportFolder,
                        marketReportFolder,
                        marketReportFileName,
                        verbose);

    }






  }

  return 0;
}
